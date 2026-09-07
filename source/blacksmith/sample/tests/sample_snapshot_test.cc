// sample::snapshot end to end. Prints the same picture the UI draws.
//
// Checks:
//   * sample::snapshot() fills all three parts in one call
//   * the parts agree: core count matches topology, the GPU ceiling fits
//     inside physical memory
//   * the one number that panics the machine, wired against the ceiling,
//     can be computed from a single Snapshot

#include "model/snapshot.hh"
#include "sample/sample.hh"

#include "check/check.hh"

#include <chrono>
#include <cstdio>

static void spin(int ms) {
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    volatile unsigned sink = 0;
    while (std::chrono::steady_clock::now() < until) sink = sink + 1;
}

// Clamped, so a NaN or an over-full gauge from a half-done sampler draws an
// empty or a full bar instead of aborting on the double-to-int conversion.
static void bar(double fraction, int cells) {
    if (!(fraction > 0.0)) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    const int filled = static_cast<int>(fraction * cells);
    for (int k = 0; k < cells; ++k) std::printf("%s", k < filled ? "#" : ".");
}

int main() {
    using namespace blacksmith;
    using check::gb;

    model::Snapshot a, b;
    sample::snapshot(a);
    spin(300);
    sample::snapshot(b);

    const model::CpuTopology topo = sample::cpuTopology();
    const std::vector<double> pct = model::perCorePercent(a.cpu, b.cpu);

    std::printf("  blacksmith . snapshot\n\n");
    std::printf("  cpu   P=%u  E=%u\n", topo.performance, topo.efficiency);
    for (std::size_t i = 0; i < pct.size(); ++i) {
        std::printf("  %c%-2zu  ", i < topo.efficiency ? 'E' : 'P', i);
        bar(pct[i] / 100.0, 20);
        std::printf("  %5.1f%%\n", pct[i]);
    }

    const model::Ledger& m = b.memory;
    std::printf("\n  memory        %6.2f GB total\n", gb(m.total));
    std::printf("  free          %6.2f GB\n", gb(m.free));
    std::printf("  active        %6.2f GB\n", gb(m.active));
    std::printf("  inactive      %6.2f GB\n", gb(m.inactive));
    std::printf("  wired         %6.2f GB\n", gb(m.wired));
    std::printf("  compressed    %6.2f GB\n", gb(m.compressed));
    std::printf("  swap          %6.2f GB of %.2f\n", gb(m.swapUsed), gb(m.swapTotal));
    std::printf("  pressure      %d\n", m.pressure);

    const double wiredOfCeiling =
        b.gpuCeiling ? static_cast<double>(m.wired) / static_cast<double>(b.gpuCeiling) : 0.0;
    std::printf("\n  gpu ceiling   %6.2f GB\n", gb(b.gpuCeiling));
    std::printf("  wired/ceiling ");
    bar(wiredOfCeiling, 30);
    std::printf("  %4.0f%%%s\n\n", 100.0 * wiredOfCeiling, wiredOfCeiling > 0.9 ? "  !! red band" : "");

    CHECK(b.cpu.cores.size() == topo.performance + topo.efficiency);
    CHECK(pct.size() == b.cpu.cores.size());
    CHECK(m.total > 0);
    CHECK(b.gpuCeiling > 0 && b.gpuCeiling < m.total);
    CHECK(m.wired < b.gpuCeiling); // above the ceiling the kernel starts killing processes

    return DONE();
}
