#include "matter_adapter.hpp"
#include <esp_log.h>
#include <platform/CHIPDeviceLayer.h>

namespace {
using namespace esp_matter;
using namespace chip::app::Clusters;
constexpr const char *TAG = "ventilation";
ventilation::Controller *controller = nullptr;
uint16_t fan_endpoint = 0;
bool publishing = false;

esp_err_t publish(uint32_t attribute_id, esp_matter_attr_val_t value) {
    return attribute::update(fan_endpoint, FanControl::Id, attribute_id, &value);
}

// Run on the Matter event loop after its own attribute callbacks complete.
// This prevents feedback loops while synchronizing mode, setting and current.
void apply_request(intptr_t percent) {
    if (!controller->set_percent(static_cast<unsigned>(percent))) {
        ESP_LOGE(TAG, "Output rejected request; simulated current unchanged");
        return;
    }
    const auto stage = controller->stage();
    const uint8_t current = ventilation::percent_from_stage(stage);
    const uint8_t mode = current == 0 ? 0 : current <= 25 ? 1 : current <= 50 ? 2 : 3;
    publishing = true;
    const auto setting_error = publish(FanControl::Attributes::PercentSetting::Id,
                                       esp_matter_nullable_uint8(current));
    const auto mode_error = publish(FanControl::Attributes::FanMode::Id, esp_matter_enum8(mode));
    const auto current_error = publish(FanControl::Attributes::PercentCurrent::Id, esp_matter_uint8(current));
    publishing = false;
    if (setting_error != ESP_OK || mode_error != ESP_OK || current_error != ESP_OK) {
        ESP_LOGE(TAG, "Matter state synchronization failed: %d/%d/%d",
                 setting_error, mode_error, current_error);
    }
    ESP_LOGI(TAG, "SIMULATION: stage=%u logical_percent=%u; no hardware output",
             static_cast<unsigned>(stage), static_cast<unsigned>(current));
}
}

esp_err_t create_ventilation_endpoint(esp_matter::node_t *node,
                                      ventilation::Controller &target) {
    controller = &target;
    esp_matter::endpoint::fan::config_t config;
    config.fan_control.fan_mode_sequence = 0; // Off / Low / Medium / High; no Auto.
    config.fan_control.fan_mode = 0;
    config.fan_control.percent_setting = 0;
    config.fan_control.percent_current = 0;
    auto *endpoint = esp_matter::endpoint::fan::create(node, &config,
        esp_matter::ENDPOINT_FLAG_NONE, nullptr);
    if (!endpoint) return ESP_ERR_NO_MEM;
    fan_endpoint = esp_matter::endpoint::get_id(endpoint);
    return ESP_OK;
}

esp_err_t ventilation_attribute_callback(esp_matter::attribute::callback_type_t type,
    uint16_t endpoint, uint32_t cluster, uint32_t attribute_id,
    esp_matter_attr_val_t *value, void *) {
    using namespace esp_matter;
    using namespace chip::app::Clusters;
    if (publishing || endpoint != fan_endpoint || cluster != FanControl::Id) return ESP_OK;
    if (attribute_id != FanControl::Attributes::PercentSetting::Id &&
        attribute_id != FanControl::Attributes::FanMode::Id) return ESP_OK;
    if (!value) return ESP_ERR_INVALID_ARG;
    const auto raw = value->val.u8;
    const bool is_percent = attribute_id == FanControl::Attributes::PercentSetting::Id;
    if ((is_percent && raw > 100) || (!is_percent && raw > 3)) return ESP_ERR_INVALID_ARG;
    if (type == attribute::POST_UPDATE) {
        // FanMode has three named speeds; four stages are selectable via percent.
        const unsigned percent = is_percent ? raw : raw == 0 ? 0 : raw == 1 ? 25 : raw == 2 ? 50 : 100;
        auto error = chip::DeviceLayer::PlatformMgr().ScheduleWork(apply_request, percent);
        return error == CHIP_NO_ERROR ? ESP_OK : ESP_FAIL;
    }
    return ESP_OK;
}
