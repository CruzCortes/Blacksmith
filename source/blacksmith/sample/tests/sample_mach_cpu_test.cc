// sample::cpu against the live machine.
//
// Checks:
//   * one CoreTicks per online core, and every core has ticked
//   * a second call into the same struct does not change its size
//   * with perCorePercent, a core that was spun shows up busy
//   * under ASan, a mismatched deallocator for the Mach buffer is reported here
//
// Apple Silicon numbers the E cluster first: cores 0..E-1 are E, the rest
// are P. The labels below rely on that, as does the UI.

#include "model/cpu.hh"
#include "sample/sample.hh"

#include "check/check.hh"

#include <chrono>
#include <cmath>
#include <cstdint>
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

    const auto online = static_cast<std::size_t>(sysconf(_SC_NPROCESSORS_ONLN));
    const model::CpuTopology topo = sample::cpuTopology();

    model::CpuSample before;
    sample::cpu(before);
    CHECK(before.cores.size() == online);
    for (const model::CoreTicks& c : before.cores) {
        const std::uint64_t total = std::uint64_t{c.user} + c.system + c.idle + c.nice;
        CHECK(total > 0); // a core that has never ticked is a stride bug
    }

    spin(300);

    model::CpuSample after;
    sample::cpu(after);
    CHECK(after.cores.size() == before.cores.size());

    const std::vector<double> pct = model::perCorePercent(before, after);
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

    bool someoneBusy = false;
    for (double p : pct) someoneBusy = someoneBusy || p > 50.0;
    CHECK(someoneBusy);

    // Reuse the caller's struct: size must not drift.
    sample::cpu(before);
    CHECK(before.cores.size() == online);

    // A leak or a double free shows here under ASan, not in the checks.
    for (int i = 0; i < 200; ++i) sample::cpu(after);

    return DONE();
}
