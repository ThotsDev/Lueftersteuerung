#include "matter_adapter.hpp"
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_matter_console.h>
#include <setup_payload/OnboardingCodesUtil.h>

namespace {
constexpr const char *TAG = "network";
ventilation::MockFanOutput output;
ventilation::Controller controller(output);

void network_event(const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t) {
    using namespace chip::DeviceLayer;
    switch (event->Type) {
    case DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "IP address changed; Matter manages WLAN/IPv6/mDNS");
        break;
    case DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Matter commissioning complete");
        break;
    case DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGW(TAG, "Commissioning timed out; check phone, WLAN and IPv6");
        break;
    case DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window open");
        break;
    case DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;
    default:
        break;
    }
}
}

extern "C" void app_main() {
    // Preserve existing credentials on NVS failure; never auto-erase a fabric.
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGW(TAG, "Development firmware: mock output only");
    esp_matter::node::config_t config;
    auto *node = esp_matter::node::create(&config, ventilation_attribute_callback, nullptr);
    ESP_ERROR_CHECK(node ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(create_ventilation_endpoint(node, controller));
    ESP_ERROR_CHECK(esp_matter::start(network_event));
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::init();
#endif
}
