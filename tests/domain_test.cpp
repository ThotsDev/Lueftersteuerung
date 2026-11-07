#include "ventilation.hpp"
#include <cassert>
#include <climits>
#include <iostream>
using namespace ventilation;

class RejectingOutput final : public FanOutput {
public:
    bool reject = false;
    bool apply(Stage) override { return !reject; }
};

int main() {
    MockFanOutput mock;
    Controller control(mock);
    for (unsigned p = 0; p <= 100; ++p) {
        assert(control.set_percent(p));
        const unsigned level = static_cast<unsigned>(control.stage());
        assert(level <= 4);
        assert(mock.simulated_stage() == control.stage());
        if (p == 0) assert(level == 0);
        else { assert((level - 1) * 25 < p); assert(p <= level * 25); }
    }
    for (unsigned p : {101u, 255u, UINT_MAX}) {
        assert(!control.set_percent(p));
        assert(control.stage() == Stage::four);
    }
    assert(control.set_percent(0));
    assert(control.stage() == Stage::off);
    RejectingOutput driver;
    Controller failing(driver);
    assert(failing.set_percent(26));
    driver.reject = true;
    assert(!failing.set_percent(100));
    assert(failing.stage() == Stage::two);
    std::cout << "PASS: 101 valid percentages, invalid requests, off, output failure\n";
}
