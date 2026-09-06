// test_cpu -- covers dev/00 tasks 0.2 to 0.7.
//
// What it proves:
//   * cpuTopology() agrees with the number of online processors
//   * sampleCpu() fills one CoreTicks per core, and every core has ticked
//   * perCorePercent() is the right length and every value is 0..100
//   * a second sampleCpu() into the same struct does not change its size
//
// Apple Silicon numbers the E cluster first: cores 0..E-1 are E, the rest
// are P. The labels below rely on that. Your TUI will too.

#include "check.hpp"
#include "cpu.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>

#include <unistd.h>

// Burn one core for `ms` so at least one bar is not idle.
static void spin(int ms) {
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    volatile unsigned sink = 0;
    while (std::chrono::steady_clock::now() < until) sink = sink + 1;
}

int main() {
    using namespace blacksmith;

    const CpuTopology topo = cpuTopology();
    const auto online = static_cast<unsigned>(sysconf(_SC_NPROCESSORS_ONLN));
    std::printf("  topology   P=%u  E=%u   online=%u\n", topo.performance, topo.efficiency, online);
    CHECK(topo.performance > 0);
    CHECK(topo.efficiency > 0);
    CHECK(topo.performance + topo.efficiency == online);

    CpuSample before;
    sampleCpu(before);
    CHECK(before.cores.size() == online);
    for (const CoreTicks& c : before.cores) {
        std::uint64_t total = 0;
        for (std::uint64_t t : c.state) total += t;
        CHECK(total > 0); // a core that has never ticked is a stride bug
    }

    spin(300);

    CpuSample after;
    sampleCpu(after);
    CHECK(after.cores.size() == before.cores.size());

    const std::vector<double> pct = perCorePercent(before, after);
    CHECK(pct.size() == before.cores.size());

    std::printf("\n");
    for (std::size_t i = 0; i < pct.size(); ++i) {
        const char cluster = i < topo.efficiency ? 'E' : 'P';
        const int filled = static_cast<int>(pct[i] / 5.0); // 20 cells
        std::printf("  %c%-2zu  %5.1f%%  ", cluster, i, pct[i]);
        for (int k = 0; k < 20; ++k) std::printf("%s", k < filled ? "#" : ".");
        std::printf("\n");
        CHECK(std::isfinite(pct[i]));
        CHECK(pct[i] >= 0.0 && pct[i] <= 100.0);
    }
    std::printf("\n");

    // At least one core did the spin.
    bool someoneBusy = false;
    for (double p : pct) someoneBusy = someoneBusy || p > 50.0;
    CHECK(someoneBusy);

    // Reuse the caller's struct: size must not drift.
    sampleCpu(before);
    CHECK(before.cores.size() == online);

    return DONE();
}
