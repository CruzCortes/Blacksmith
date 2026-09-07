// perCorePercent on hand-made tick counts.
//
// No OS calls, so every expected value is known before the test runs. This
// includes the 32-bit counter wrap, which a live machine takes 497 days to
// produce.

#include "model/cpu.hh"

#include "check/check.hh"

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

using namespace blacksmith::model;

static CpuSample one(std::uint32_t user, std::uint32_t system, std::uint32_t idle, std::uint32_t nice) {
    CpuSample s;
    s.cores.push_back(CoreTicks{user, system, idle, nice});
    return s;
}

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

// First percent of a result, or NaN if the result is empty. An empty vector
// from a half-written perCorePercent must show as a FAIL line, not as a
// crash on p[0].
static double first(const std::vector<double>& p) {
    return p.empty() ? std::numeric_limits<double>::quiet_NaN() : p[0];
}

int main() {
    // 100 busy ticks out of 500 elapsed -> 20 percent
    {
        const auto p = perCorePercent(one(100, 50, 850, 0), one(150, 100, 1250, 0));
        CHECK(p.size() == 1);
        CHECK(near(first(p), 20.0));
    }
    // nothing moved -> 0, not NaN
    {
        const auto p = perCorePercent(one(1, 1, 1, 1), one(1, 1, 1, 1));
        CHECK(std::isfinite(first(p)));
        CHECK(first(p) == 0.0);
    }
    // only idle moved -> 0
    CHECK(first(perCorePercent(one(0, 0, 0, 0), one(0, 0, 40, 0))) == 0.0);
    // only busy moved -> 100
    CHECK(first(perCorePercent(one(0, 0, 0, 0), one(10, 0, 0, 0))) == 100.0);
    // nice is busy time too
    CHECK(near(first(perCorePercent(one(0, 0, 0, 0), one(0, 0, 50, 50))), 50.0));
    // the 32-bit counter wrapped between samples: 4294967290 -> 5 is 11 ticks, all busy
    CHECK(first(perCorePercent(one(4294967290u, 0, 0, 0), one(5u, 0, 0, 0))) == 100.0);

    // twelve cores in, twelve out, order preserved
    {
        CpuSample a, b;
        for (std::uint32_t i = 0; i < 12; ++i) {
            a.cores.push_back(CoreTicks{0, 0, 0, 0});
            b.cores.push_back(CoreTicks{i, 0, 12 - i, 0}); // busy i of 12
        }
        const auto p = perCorePercent(a, b);
        CHECK(p.size() == 12);
        if (p.size() == 12) {
            CHECK(p[0] == 0.0);
            CHECK(near(p[6], 50.0));
            CHECK(near(p[11], 100.0 * 11.0 / 12.0));
        }
    }
    return DONE();
}
