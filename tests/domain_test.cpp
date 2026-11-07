#include "ventilation.hpp"
#include <cassert>
#include <climits>
#include <iostream>
using namespace ventilation;

class RejectingOutput final : public FanOutput {
public:
    bool reject = false;
    unsigned writes = 0;
    bool apply(const Frame &) override { ++writes; return !reject; }
};

unsigned supply_count(const Frame &frame) {
    unsigned count = 0;
    for (auto direction : frame.directions) if (direction == Direction::supply) ++count;
    return count;
}

void expect_inverse(const Frame &a, const Frame &b) {
    assert(a.stage == b.stage);
    for (unsigned i = 0; i < room_count; ++i) assert(a.directions[i] != b.directions[i]);
    assert(supply_count(a) == 2 && supply_count(b) == 2);
}

int main() {
    MockFanOutput mock;
    Controller control(mock);
    for (unsigned p = 0; p <= 100; ++p) {
        assert(control.set_percent(p));
        const unsigned level = static_cast<unsigned>(control.stage());
        assert(level <= 3);
        assert(mock.simulated_stage() == control.stage());
        if (p == 0) assert(level == 0);
        else { assert((level - 1) * 100 < p * 3); assert(p * 3 <= level * 100); }
    }
    for (unsigned p : {101u, 255u, UINT_MAX}) {
        assert(!control.set_percent(p));
        assert(control.stage() == Stage::strong);
    }
    assert(control.set_percent(0));
    assert(control.stage() == Stage::off);
    RejectingOutput driver;
    Controller failing(driver);
    assert(failing.set_percent(50));
    driver.reject = true;
    assert(!failing.set_percent(100));
    assert(failing.stage() == Stage::medium);
    assert(percent_from_stage(Stage::light) == 33);
    assert(percent_from_stage(Stage::medium) == 66);
    assert(percent_from_stage(Stage::strong) == 100);

    // Every possible room assignment: exactly six valid, balanced layouts.
    unsigned valid = 0;
    for (unsigned mask = 0; mask < 256; ++mask) {
        const auto before = control.frame();
        const auto previous = control.supply_mask();
        if (control.set_layout(mask, 100)) {
            ++valid;
            assert(supply_count(control.frame()) == 2);
            assert(layout_index(mask) < 6);
        } else {
            assert(control.frame() == before);
            assert(control.supply_mask() == previous);
        }
    }
    assert(valid == 6);
    assert(!control.set_layout(UINT_MAX, 100));
    assert(!control.set_stage(static_cast<Stage>(4), 0));
    assert(!control.set_mode(static_cast<Mode>(2), 0));

    // Cross ventilation stays in the configured direction indefinitely.
    assert(control.set_layout(default_supply_mask, 0));
    assert(control.set_stage(Stage::light, 1000));
    const auto standard = control.frame();
    assert(standard.directions[0] == Direction::supply);
    assert(standard.directions[1] == Direction::supply);
    assert(standard.directions[2] == Direction::exhaust);
    assert(standard.directions[3] == Direction::exhaust);
    assert(control.tick(10000000));
    assert(control.frame() == standard);

    // All stages use their own interval and remain balanced in both phases.
    for (auto stage : {Stage::light, Stage::medium, Stage::strong}) {
        MockFanOutput fan;
        Controller c(fan);
        assert(c.set_mode(Mode::alternating, 100));
        assert(c.set_stage(stage, 100));
        const auto initial = c.frame();
        const auto interval = reversal_interval_ms(stage);
        assert(c.tick(100 + interval - 1));
        assert(c.frame() == initial);
        // Repeated writes of the same stage must not postpone a reversal.
        assert(c.set_stage(stage, 100 + interval - 1));
        assert(c.tick(100 + interval));
        expect_inverse(initial, c.frame());
        assert(c.tick(100 + 2 * interval));
        assert(c.frame() == initial);
        // Late polling catches up by parity instead of rapidly cycling outputs.
        assert(c.tick(100 + 5 * interval));
        expect_inverse(initial, c.frame());
        assert(c.set_stage(Stage::off, 100 + 5 * interval));
        const auto stopped = c.frame();
        assert(c.tick(100 + 100 * interval));
        assert(c.frame() == stopped);
        assert(c.set_stage(stage, 100 + 100 * interval));
        assert(c.frame() == initial);
        assert(c.tick(100 + 101 * interval));
        expect_inverse(initial, c.frame());
        assert(c.set_mode(Mode::cross, 100 + 101 * interval));
        assert(c.frame() == initial);
    }

    // A configuration change always establishes the new baseline atomically.
    MockFanOutput rooms;
    Controller c(rooms);
    assert(c.set_stage(Stage::strong, 0));
    assert(c.set_mode(Mode::alternating, 0));
    assert(c.tick(50000));
    assert(c.reversed());
    assert(c.set_layout(0x0c, 50001));
    assert(!c.reversed());
    assert(c.frame().directions[0] == Direction::exhaust);
    assert(c.frame().directions[2] == Direction::supply);
    assert(c.tick(100000));
    assert(!c.reversed());
    assert(c.tick(100001));
    assert(c.reversed());

    // millis() rollover must not suppress or advance the reversal.
    MockFanOutput wrapOutput;
    Controller wrap(wrapOutput);
    constexpr std::uint32_t start = UINT_MAX - 10000;
    assert(wrap.set_mode(Mode::alternating, start));
    assert(wrap.set_stage(Stage::strong, start));
    const auto wrapInitial = wrap.frame();
    assert(wrap.tick(start + 49999u));
    assert(wrap.frame() == wrapInitial);
    assert(wrap.tick(start + 50000u));
    expect_inverse(wrapInitial, wrap.frame());

    // Rejected output changes never publish a partially changed configuration.
    RejectingOutput rejecting;
    Controller rollback(rejecting);
    assert(rollback.set_mode(Mode::alternating, 0));
    assert(rollback.set_stage(Stage::strong, 0));
    const auto beforeFailure = rollback.frame();
    rejecting.reject = true;
    assert(!rollback.tick(50000));
    assert(!rollback.reversed());
    assert(rollback.frame() == beforeFailure);
    assert(!rollback.set_layout(0x0c, 50000));
    assert(rollback.supply_mask() == default_supply_mask);
    rejecting.reject = false;
    assert(rollback.tick(50000));
    expect_inverse(beforeFailure, rollback.frame());
    std::cout << "PASS: stages, all balanced layouts, reversal timers, rollover and output rollback\n";
}
