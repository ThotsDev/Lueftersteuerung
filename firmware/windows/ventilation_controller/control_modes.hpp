#pragma once
#include <Matter.h>
#include <app/clusters/mode-select-server/supported-modes-manager.h>
#include <array>
#include <cstdio>
#include <functional>

enum class ControlRequestType : uint8_t { percent, stage, operation, layout };
struct ControlRequest { ControlRequestType type; uint8_t value; };

// One shared manager dispatches the standard ModeSelect lists by endpoint ID.
// Each endpoint is a proper Mode Select device, not a simulated socket switch.
class ControlModeOptions final : public chip::app::Clusters::ModeSelect::SupportedModesManager {
public:
    enum class Kind : uint8_t { stage, operation, layout };
    using Option = chip::app::Clusters::ModeSelect::Structs::ModeOptionStruct::Type;
    ControlModeOptions() {
        setLabels(stages_, std::array<const char *, 4>{{"Aus", "Leicht", "Mittel", "Stark"}});
        setLabels(operations_, std::array<const char *, 2>{{"Durchzug", "Wechsellüftung"}});
        setLabels(layouts_, std::array<const char *, 6>{{
            "Wohnzimmer + Schlafzimmer", "Wohnzimmer + Bad", "Wohnzimmer + Büro",
            "Schlafzimmer + Bad", "Schlafzimmer + Büro", "Bad + Büro"}});
    }
    void bind(Kind kind, uint16_t endpoint) { endpoints_[static_cast<unsigned>(kind)] = endpoint; }
    ModeOptionsProvider getModeOptionsProvider(chip::EndpointId endpoint) const override {
        if (endpoint == endpoints_[0]) return {stages_.data(), stages_.data() + stages_.size()};
        if (endpoint == endpoints_[1]) return {operations_.data(), operations_.data() + operations_.size()};
        if (endpoint == endpoints_[2]) return {layouts_.data(), layouts_.data() + layouts_.size()};
        return {};
    }
    chip::Protocols::InteractionModel::Status getModeOptionByMode(chip::EndpointId endpoint,
        uint8_t mode, const Option **result) const override {
        const auto options = getModeOptionsProvider(endpoint);
        for (auto option = options.begin(); option != options.end(); ++option) {
            if (option->mode == mode) { *result = option; return chip::Protocols::InteractionModel::Status::Success; }
        }
        *result = nullptr;
        return chip::Protocols::InteractionModel::Status::ConstraintError;
    }
private:
    template <size_t N>
    static void setLabels(std::array<Option, N> &options, const std::array<const char *, N> &labels) {
        for (size_t i = 0; i < N; ++i) {
            options[i].label = chip::CharSpan::fromCharString(labels[i]);
            options[i].mode = static_cast<uint8_t>(i);
        }
    }
    std::array<uint16_t, 3> endpoints_{{0xffff, 0xffff, 0xffff}};
    std::array<Option, 4> stages_;
    std::array<Option, 2> operations_;
    std::array<Option, 6> layouts_;
};

class ControlModeSelector final : public MatterEndPoint {
public:
    bool begin(ControlModeOptions &options, ControlModeOptions::Kind kind, const char *description,
               uint8_t initial, std::function<bool(uint8_t)> callback) {
        esp_matter::endpoint::mode_select::config_t config;
        std::snprintf(config.mode_select.description, sizeof(config.mode_select.description), "%s", description);
        config.mode_select.current_mode = initial;
        config.mode_select.delegate = &options;
        auto *endpoint = esp_matter::endpoint::mode_select::create(esp_matter::node::get(), &config,
            esp_matter::ENDPOINT_FLAG_NONE, this);
        if (!endpoint) return false;
        setEndPointId(esp_matter::endpoint::get_id(endpoint));
        options.bind(kind, getEndPointId());
        count_ = kind == ControlModeOptions::Kind::stage ? 4 : kind == ControlModeOptions::Kind::operation ? 2 : 6;
        callback_ = callback;
        return true;
    }
    bool publish(uint8_t mode) {
        auto value = esp_matter_uint8(mode);
        return updateAttributeVal(chip::app::Clusters::ModeSelect::Id,
            chip::app::Clusters::ModeSelect::Attributes::CurrentMode::Id, &value);
    }
    bool attributeChangeCB(uint16_t endpoint, uint32_t cluster, uint32_t attribute,
                           esp_matter_attr_val_t *value) override {
        if (endpoint == getEndPointId() && cluster == chip::app::Clusters::ModeSelect::Id &&
            attribute == chip::app::Clusters::ModeSelect::Attributes::CurrentMode::Id) {
            return value && value->val.u8 < count_ && (!callback_ || callback_(value->val.u8));
        }
        return true;
    }
private:
    uint8_t count_ = 0;
    std::function<bool(uint8_t)> callback_;
};
