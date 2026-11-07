#pragma once
#include <cstdint>

namespace ventilation {
enum class Stage : std::uint8_t { off = 0, one, two, three, four };

// Logical levels only: no assumptions about electrical signaling or airflow.
constexpr bool valid_percent(unsigned percent) { return percent <= 100; }
constexpr Stage stage_from_percent(unsigned percent) {
    return percent == 0 ? Stage::off
        : static_cast<Stage>((percent + 24) / 25);
}
constexpr std::uint8_t percent_from_stage(Stage stage) {
    return static_cast<std::uint8_t>(static_cast<unsigned>(stage) * 25);
}

class FanOutput {
public:
    virtual ~FanOutput() = default;
    virtual bool apply(Stage stage) = 0;
};

class Controller {
public:
    explicit Controller(FanOutput &output) : output_(output) {}
    bool set_percent(unsigned percent) {
        if (!valid_percent(percent)) return false;
        const Stage next = stage_from_percent(percent);
        if (!output_.apply(next)) return false;
        stage_ = next;
        return true;
    }
    Stage stage() const { return stage_; }
private:
    FanOutput &output_;
    Stage stage_ = Stage::off;
};

class MockFanOutput final : public FanOutput {
public:
    bool apply(Stage stage) override { simulated_ = stage; return true; }
    Stage simulated_stage() const { return simulated_; }
private:
    Stage simulated_ = Stage::off;
};
} // namespace ventilation
