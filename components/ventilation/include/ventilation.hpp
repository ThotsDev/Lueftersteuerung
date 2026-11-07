#pragma once
#include <array>
#include <cstdint>

namespace ventilation {
enum class Stage : std::uint8_t { off = 0, light, medium, strong };
enum class Mode : std::uint8_t { cross = 0, alternating };
enum class Room : std::uint8_t { living = 0, bedroom, bathroom, office };
enum class Direction : std::uint8_t { exhaust = 0, supply };
constexpr unsigned room_count = 4;
constexpr std::uint8_t default_supply_mask = 0x03; // Living room + bedroom.
constexpr std::array<std::uint8_t, 6> supply_layouts{{0x03, 0x05, 0x09, 0x06, 0x0a, 0x0c}};

constexpr bool valid_layout(unsigned mask) {
    if (mask > 15) return false;
    unsigned count = 0;
    for (unsigned i = 0; i < room_count; ++i) count += (mask >> i) & 1u;
    return count == 2;
}
constexpr unsigned layout_index(unsigned mask) {
    for (unsigned i = 0; i < supply_layouts.size(); ++i) if (supply_layouts[i] == mask) return i;
    return supply_layouts.size();
}

// Logical levels only: no assumptions about electrical signaling or airflow.
constexpr bool valid_percent(unsigned percent) { return percent <= 100; }
constexpr Stage stage_from_percent(unsigned percent) {
    return percent == 0 ? Stage::off
        : static_cast<Stage>((percent * 3 + 99) / 100);
}
constexpr std::uint8_t percent_from_stage(Stage stage) {
    return static_cast<std::uint8_t>(static_cast<unsigned>(stage) * 100 / 3);
}

// Simulation timings chosen inside the VC40 H manual's 50..70 s range.
// They are not a measured mapping of the original controller's speed stages.
constexpr std::uint32_t reversal_interval_ms(Stage stage) {
    return stage == Stage::off ? 0 : stage == Stage::light ? 70000 : stage == Stage::medium ? 60000 : 50000;
}

struct Frame {
    Stage stage = Stage::off;
    std::array<Direction, room_count> directions{{Direction::supply, Direction::supply,
                                                Direction::exhaust, Direction::exhaust}};
};
inline bool operator==(const Frame &left, const Frame &right) {
    return left.stage == right.stage && left.directions == right.directions;
}

class FanOutput {
public:
    virtual ~FanOutput() = default;
    // One complete balanced frame, never four partially updated room outputs.
    virtual bool apply(const Frame &frame) = 0;
};

class Controller {
public:
    explicit Controller(FanOutput &output) : output_(output) {}
    bool set_percent(unsigned percent, std::uint32_t now = 0) {
        if (!valid_percent(percent)) return false;
        return set_stage(stage_from_percent(percent), now);
    }
    bool set_stage(Stage stage, std::uint32_t now) {
        if (static_cast<unsigned>(stage) > 3) return false;
        if (stage == frame_.stage) return true;
        const bool reverse = stage == Stage::off || frame_.stage == Stage::off ? false : reversed_;
        return commit(stage, mode_, supply_mask_, reverse, now);
    }
    bool set_mode(Mode mode, std::uint32_t now) {
        if (mode != Mode::cross && mode != Mode::alternating) return false;
        if (mode == mode_) return true;
        return commit(stage(), mode, supply_mask_, false, now);
    }
    bool set_layout(unsigned mask, std::uint32_t now) {
        if (!valid_layout(mask)) return false;
        if (mask == supply_mask_) return true;
        return commit(stage(), mode_, static_cast<std::uint8_t>(mask), false, now);
    }
    bool tick(std::uint32_t now) {
        if (stage() == Stage::off || mode_ != Mode::alternating) return true;
        const auto interval = reversal_interval_ms(stage());
        // Unsigned subtraction also works across millis() rollover.
        const auto elapsed = static_cast<std::uint32_t>(now - phase_start_);
        const auto periods = elapsed / interval;
        if (periods == 0) return true;
        return commit(stage(), mode_, supply_mask_, reversed_ != ((periods & 1u) != 0),
                      phase_start_ + periods * interval);
    }
    Stage stage() const { return frame_.stage; }
    Mode mode() const { return mode_; }
    std::uint8_t supply_mask() const { return supply_mask_; }
    bool reversed() const { return reversed_; }
    const Frame &frame() const { return frame_; }
private:
    bool commit(Stage stage, Mode mode, std::uint8_t mask, bool reverse, std::uint32_t start) {
        Frame next;
        next.stage = stage;
        for (unsigned i = 0; i < room_count; ++i) {
            const bool supply = (((mask >> i) & 1u) != 0) != reverse;
            next.directions[i] = supply ? Direction::supply : Direction::exhaust;
        }
        if (!(next == frame_) && !output_.apply(next)) return false;
        frame_ = next;
        mode_ = mode;
        supply_mask_ = mask;
        reversed_ = reverse;
        phase_start_ = start;
        return true;
    }
    FanOutput &output_;
    Frame frame_;
    Mode mode_ = Mode::cross;
    std::uint8_t supply_mask_ = default_supply_mask;
    bool reversed_ = false;
    std::uint32_t phase_start_ = 0;
};

class MockFanOutput final : public FanOutput {
public:
    bool apply(const Frame &frame) override { simulated_ = frame; return true; }
    Stage simulated_stage() const { return simulated_.stage; }
    const Frame &simulated_frame() const { return simulated_; }
private:
    Frame simulated_;
};
} // namespace ventilation
