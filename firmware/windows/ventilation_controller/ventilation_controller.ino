#include <Arduino.h>
#include <Matter.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <platform/CHIPDeviceLayer.h>
#include <ventilation.hpp>
#include <atomic>
#include "control_modes.hpp"

#ifndef CONFIG_ESP_MATTER_ENABLE_DATA_MODEL
#error "This ESP32 core does not contain the required precompiled Matter libraries."
#endif

namespace {
MatterFan fan;
ControlModeOptions modeOptions;
ControlModeSelector stageSelector;
ControlModeSelector operationSelector;
ControlModeSelector layoutSelector;
ventilation::MockFanOutput output;
ventilation::Controller controller(output);
QueueHandle_t requests;
bool matterStarted = false;
std::atomic<bool> matterReady{false};
std::atomic<uint32_t> controlSnapshot{0};
bool publishing = false; // Accessed only on the CHIP thread.
enum class Input { command, ssid, password };
Input input = Input::command;
String line;
String pendingSsid;
bool overflow = false;

bool queueRequest(ControlRequestType type, uint8_t value) {
  // Ignore SDK startup/restoration callbacks. The local boot state is authoritative.
  if (publishing || !matterReady.load()) return true;
  const ControlRequest request{type, value};
  return xQueueSend(requests, &request, 0) == pdTRUE;
}

void publishControls(intptr_t packed) {
  const uint32_t state = static_cast<uint32_t>(packed);
  const uint8_t stage = state & 0xff;
  const uint8_t operation = (state >> 8) & 0xff;
  const uint8_t layout = (state >> 16) & 0xff;
  const uint8_t value = ventilation::percent_from_stage(static_cast<ventilation::Stage>(stage));
  const auto mode = value == 0 ? MatterFan::FAN_MODE_OFF
      : stage == 1 ? MatterFan::FAN_MODE_LOW
      : stage == 2 ? MatterFan::FAN_MODE_MEDIUM : MatterFan::FAN_MODE_HIGH;
  publishing = true;
  const bool modeOk = fan.setMode(mode);
  const bool speedOk = fan.setSpeedPercent(value);
  auto speedCurrent = esp_matter_uint8(stage);
  const bool currentOk = esp_matter::attribute::update(fan.getEndPointId(),
      chip::app::Clusters::FanControl::Id, chip::app::Clusters::FanControl::Attributes::SpeedCurrent::Id,
      &speedCurrent) == ESP_OK;
  const bool stageOk = stageSelector.publish(stage);
  const bool operationOk = operationSelector.publish(operation);
  const bool layoutOk = layoutSelector.publish(layout);
  publishing = false;
  if (!modeOk || !speedOk || !currentOk || !stageOk || !operationOk || !layoutOk) {
    Serial.println("ERROR: Matter state publication failed");
  }
}

void schedulePublication() {
  const uint32_t packed = static_cast<unsigned>(controller.stage()) |
      (static_cast<unsigned>(controller.mode()) << 8) |
      (ventilation::layout_index(controller.supply_mask()) << 16);
  controlSnapshot.store(packed);
  if (!matterReady.load()) return;
  if (chip::DeviceLayer::PlatformMgr().ScheduleWork(publishControls, packed) != CHIP_NO_ERROR) {
    Serial.println("ERROR: scheduling Matter state publication failed");
  }
}

bool saveConfiguration() {
  Preferences prefs;
  if (!prefs.begin("vent-config", false)) return false;
  const uint32_t packed = (1u << 16) | (static_cast<unsigned>(controller.mode()) << 8) | controller.supply_mask();
  const bool ok = prefs.putUInt("config", packed) == sizeof(packed);
  prefs.end();
  return ok;
}

void loadConfiguration() {
  uint32_t config = (1u << 16) | (1u << 8) | ventilation::default_supply_mask;
  Preferences prefs;
  if (prefs.begin("vent-config", true)) {
    config = prefs.getUInt("config", config);
    prefs.end();
  }
  if ((config >> 16) != 1 || ((config >> 8) & 0xff) > 1 || !ventilation::valid_layout(config & 0xff)) {
    Serial.println("ERROR: invalid saved configuration; using balanced defaults");
    config = (1u << 16) | (1u << 8) | ventilation::default_supply_mask;
  }
  controller.set_layout(config & 0xff, millis());
  controller.set_mode(static_cast<ventilation::Mode>((config >> 8) & 0xff), millis());
}

void printState() {
  const char *names[] = {"Aus", "Leicht", "Mittel", "Stark"};
  const char *rooms[] = {"Wohnzimmer", "Schlafzimmer", "Bad", "Büro"};
  Serial.printf("STATE STAGE=%u NAME=%s MODE=%s SUPPLY_MASK=%u REVERSED=%u INTERVAL_MS=%lu SIMULATION=1\n",
      static_cast<unsigned>(controller.stage()), names[static_cast<unsigned>(controller.stage())],
      controller.mode() == ventilation::Mode::cross ? "cross" : "alternating",
      controller.supply_mask(), controller.reversed(),
      static_cast<unsigned long>(ventilation::reversal_interval_ms(controller.stage())));
  for (unsigned i = 0; i < ventilation::room_count; ++i) {
    Serial.printf("ROOM=%s DIRECTION=%s DEFAULT=%s ACTIVE=%u\n", rooms[i],
        controller.frame().directions[i] == ventilation::Direction::supply ? "Zuluft" : "Abluft",
        (controller.supply_mask() & (1u << i)) ? "Zuluft" : "Abluft",
        controller.stage() != ventilation::Stage::off);
  }
}

bool applyRequest(ControlRequestType type, uint8_t value) {
  const auto now = millis();
  const auto oldMode = controller.mode();
  const auto oldMask = controller.supply_mask();
  bool ok = false;
  switch (type) {
    case ControlRequestType::percent: ok = controller.set_percent(value, now); break;
    case ControlRequestType::stage: ok = controller.set_stage(static_cast<ventilation::Stage>(value), now); break;
    case ControlRequestType::operation: ok = controller.set_mode(static_cast<ventilation::Mode>(value), now); break;
    case ControlRequestType::layout:
      ok = value < ventilation::supply_layouts.size() && controller.set_layout(ventilation::supply_layouts[value], now);
      break;
  }
  if (!ok) { Serial.println("ERROR: control request rejected"); return false; }
  if ((oldMode != controller.mode() || oldMask != controller.supply_mask()) && !saveConfiguration()) {
    Serial.println("ERROR: configuration active but persistence failed");
  }
  if (matterStarted) schedulePublication();
  printState();
  return true;
}

int roomIndex(const String &room) {
  if (room == "wohnzimmer") return 0;
  if (room == "schlafzimmer") return 1;
  if (room == "bad") return 2;
  if (room == "büro" || room == "buero") return 3;
  return -1;
}

bool validPasscode(uint32_t code) {
  if (code == 0 || code > 99999998 || code == 12345678 || code == 87654321) return false;
  for (uint32_t digit = 1; digit <= 9; ++digit) {
    if (code == digit * 11111111) return false;
  }
  return true;
}

bool setIdentity() {
  Preferences prefs;
  if (!prefs.begin("vent-matter", false)) return false;
  uint32_t passcode = prefs.getUInt("passcode", 0);
  uint16_t discriminator = prefs.getUShort("discriminator", 0xffff);
  if (!validPasscode(passcode)) {
    do { passcode = esp_random() % 99999998 + 1; } while (!validPasscode(passcode));
    if (prefs.putUInt("passcode", passcode) != sizeof(passcode)) { prefs.end(); return false; }
  }
  if (discriminator > 4095) {
    discriminator = esp_random() & 0x0fff;
    if (prefs.putUShort("discriminator", discriminator) != sizeof(discriminator)) { prefs.end(); return false; }
  }
  prefs.end();
  return Matter.setSetupPasscode(passcode) && Matter.setSetupDiscriminator(discriminator);
}

void printPairing() {
  if (!matterStarted) { Serial.println("PAIRING_UNAVAILABLE: connect Wi-Fi first"); return; }
  if (Matter.isDeviceCommissioned()) {
    Serial.println("ALREADY_COMMISSIONED: open a new pairing window with your Matter controller");
    return;
  }
  Serial.print("MATTER_MANUAL_CODE=");
  Serial.println(Matter.getManualPairingCode());
  Serial.print("MATTER_QR_URL=");
  Serial.println(Matter.getOnboardingQRCodeUrl());
}

void connectStoredWifi() {
  Preferences prefs;
  if (!prefs.begin("vent-wifi", true)) {
    Serial.println("WIFI_NOT_CONFIGURED: use wifi-setup");
    return;
  }
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  prefs.end();
  if (ssid.isEmpty()) { Serial.println("WIFI_NOT_CONFIGURED: use wifi-setup"); return; }
  WiFi.persistent(false);
  WiFi.setHostname("lueftungssteuerung");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.enableIPv6(true);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("WIFI_CONNECTING");
}

void handleLine(const String &value) {
  if (input == Input::ssid) {
    if (value.isEmpty() || value.length() > 32) {
      Serial.println("ERROR: SSID must contain 1..32 bytes; setup cancelled");
      input = Input::command;
      return;
    }
    pendingSsid = value;
    input = Input::password;
    Serial.println("WIFI_PASSWORD>");
    return;
  }
  if (input == Input::password) {
    input = Input::command;
    // WPA/WPA2/WPA3 passphrases; open networks and raw PSKs are not enabled here.
    if (value.length() < 8 || value.length() > 63) {
      pendingSsid = "";
      Serial.println("ERROR: password must contain 8..63 bytes; setup cancelled");
      return;
    }
    Preferences prefs;
    bool saved = prefs.begin("vent-wifi", false);
    if (saved) {
      saved = prefs.putString("ssid", pendingSsid) > 0 && prefs.putString("password", value) > 0;
      prefs.end();
    }
    pendingSsid = "";
    if (!saved) { Serial.println("ERROR: storing Wi-Fi settings failed"); return; }
    Serial.println("WIFI_SAVED: restarting");
    Serial.flush();
    delay(100);
    ESP.restart();
    return;
  }
  if (value == "wifi-setup") {
    input = Input::ssid;
    Serial.println("WIFI_SSID>");
  } else if (value == "pairing") {
    printPairing();
  } else if (value == "status") {
    Serial.printf("WIFI=%s MATTER=%s STAGE=%u SIMULATION=1\n",
      WiFi.isConnected() ? "connected" : "disconnected",
      !matterStarted ? "not-started" : Matter.isDeviceCommissioned() ? "commissioned" : "uncommissioned",
      static_cast<unsigned>(controller.stage()));
    printState();
  } else if (value.startsWith("stage ")) {
    String stage = value.substring(6); stage.toLowerCase();
    const char *labels[] = {"aus", "leicht", "mittel", "stark"};
    for (uint8_t i = 0; i < 4; ++i) {
      if (stage == labels[i] || stage == String(i)) { applyRequest(ControlRequestType::stage, i); return; }
    }
    Serial.println("ERROR: stage must be Aus, Leicht, Mittel or Stark (0..3)");
  } else if (value.startsWith("mode ")) {
    const String mode = value.substring(5);
    if (mode == "durchzug" || mode == "cross") applyRequest(ControlRequestType::operation, 0);
    else if (mode == "wechsel" || mode == "alternating") applyRequest(ControlRequestType::operation, 1);
    else Serial.println("ERROR: mode must be durchzug or wechsel");
  } else if (value.startsWith("supply ")) {
    String rooms = value.substring(7); rooms.toLowerCase();
    const int separator = rooms.indexOf(' ');
    const int first = roomIndex(rooms.substring(0, separator < 0 ? rooms.length() : separator));
    const int second = separator < 0 ? -1 : roomIndex(rooms.substring(separator + 1));
    if (first < 0 || second < 0 || first == second) {
      Serial.println("ERROR: supply needs two different rooms: wohnzimmer schlafzimmer bad buero");
    } else {
      applyRequest(ControlRequestType::layout, ventilation::layout_index((1u << first) | (1u << second)));
    }
  } else if (!value.isEmpty()) {
    Serial.println("Commands: wifi-setup | status | pairing | stage Aus/Leicht/Mittel/Stark | mode durchzug/wechsel | supply room room");
  }
}

void readConsole() {
  for (unsigned n = 0; n < 128 && Serial.available(); ++n) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (overflow) {
        input = Input::command;
        pendingSsid = "";
        Serial.println("ERROR: input too long; setup cancelled");
      } else {
        handleLine(line);
      }
      line = "";
      overflow = false;
    } else if (!overflow) {
      if (line.length() >= 96) { line = ""; overflow = true; }
      else line += c; // No echo, including during password input.
    }
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Ventilation controller: SIMULATION, no GPIO output");
  Serial.println("Commands: wifi-setup | status | pairing | stage Aus/Leicht/Mittel/Stark | mode durchzug/wechsel | supply room room");
  requests = xQueueCreate(16, sizeof(ControlRequest));
  if (!requests) { Serial.println("ERROR: request queue allocation failed"); return; }
  loadConfiguration();
  connectStoredWifi();
}

void loop() {
  readConsole();
  if (!requests) { delay(100); return; }
  if (!matterStarted && WiFi.isConnected()) {
    // This classic ESP32 uses on-network commissioning with the Arduino prebuild.
    Matter.selectNetwork(MATTER_NETWORK_WIFI, true);
    if (Matter.waitForNetwork(100)) {
      if (!fan.begin(0, MatterFan::FAN_MODE_OFF, MatterFan::FAN_MODE_SEQ_OFF_LOW_MED_HIGH) || !setIdentity()) {
        Serial.println("ERROR: Matter initialization failed; restarting");
        delay(1000);
        ESP.restart();
      }
      esp_matter::cluster::fan_control::feature::multi_speed::config_t speeds;
      speeds.speed_max = 3;
      if (esp_matter::cluster::fan_control::feature::multi_speed::add(
          esp_matter::cluster::get(fan.getEndPointId(), chip::app::Clusters::FanControl::Id), &speeds) != ESP_OK ||
          !operationSelector.begin(modeOptions, ControlModeOptions::Kind::operation, "Betriebsart",
              static_cast<uint8_t>(controller.mode()), [](uint8_t v) { return queueRequest(ControlRequestType::operation, v); }) ||
          !layoutSelector.begin(modeOptions, ControlModeOptions::Kind::layout, "Standard-Zuluft (andere Räume Abluft)",
              ventilation::layout_index(controller.supply_mask()), [](uint8_t v) { return queueRequest(ControlRequestType::layout, v); }) ||
          !stageSelector.begin(modeOptions, ControlModeOptions::Kind::stage, "Lüftungsstufe", 0,
              [](uint8_t v) { return queueRequest(ControlRequestType::stage, v); })) {
        Serial.println("ERROR: Matter control endpoint initialization failed; restarting");
        delay(1000); ESP.restart();
      }
      Matter.setDeviceName("Lueftungssteuerung Simulation");
      Matter.setProductName("Lueftungssteuerung");
      fan.onChangeSpeedPercent([](uint8_t percent) {
        return percent <= 100 && queueRequest(ControlRequestType::percent, percent);
      });
      fan.onChangeMode([](MatterFan::FanMode_t mode) {
        const uint8_t percent = mode == MatterFan::FAN_MODE_OFF ? 0
            : mode == MatterFan::FAN_MODE_LOW ? 33 : mode == MatterFan::FAN_MODE_MEDIUM ? 66 : 100;
        return queueRequest(ControlRequestType::percent, percent);
      });
      schedulePublication();
      Matter.onEvent([](matterEvent_t event, const chip::DeviceLayer::ChipDeviceEvent *) {
        if (event == MATTER_SERVER_READY) {
          publishControls(controlSnapshot.load());
          matterReady.store(true);
          Serial.println("MATTER_CONTROLS_READY");
        }
      });
      Matter.begin();
      matterStarted = true;
      schedulePublication();
      Serial.println("MATTER_STARTED: on-network Wi-Fi commissioning");
      printPairing();
    }
  }
  ControlRequest request;
  for (unsigned n = 0; n < 16 && xQueueReceive(requests, &request, 0) == pdTRUE; ++n) {
    applyRequest(request.type, request.value);
  }
  const auto before = controller.frame();
  if (!controller.tick(millis())) Serial.println("ERROR: output rejected direction reversal");
  else if (!(before == controller.frame())) printState();
  delay(10);
}
