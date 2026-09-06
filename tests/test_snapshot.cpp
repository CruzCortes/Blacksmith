// test_snapshot -- covers dev/00 task 0.11, and is the picture the TUI
// will draw in dev/03. When this output looks right to you, phase 00 is
// done in every way that matters.
//
// What it proves:
//   * sample(Snapshot&) fills all three parts in one call
//   * the parts agree with each other: core count matches topology, the
//     GPU ceiling fits inside physical memory
//   * the one number that panics the machine, wired against the ceiling,
//     can be computed from a single Snapshot

#include "check.hpp"
#include "model.hpp"

#include <chrono>
#include <cstdio>

static void spin(int ms) {
    auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    volatile unsigned sink = 0;
    while (std::chrono::steady_clock::now() < until) sink = sink + 1;
}

static void bar(double fraction, int cells) {
    const int filled = static_cast<int>(fraction * cells);
    for (int k = 0; k < cells; ++k) std::printf("%s", k < filled ? "#" : ".");
}

int main() {
    using namespace blacksmith;
    using check::gb;

    Snapshot a{}, b{};
    sample(a);
    spin(300);
    sample(b);

    const CpuTopology topo = cpuTopology();
    const std::vector<double> pct = perCorePercent(a.cpu, b.cpu);

    std::printf("  blacksmith . snapshot\n\n");
    std::printf("  cpu   P=%u  E=%u\n", topo.performance, topo.efficiency);
    for (std::size_t i = 0; i < pct.size(); ++i) {
        std::printf("  %c%-2zu  ", i < topo.efficiency ? 'E' : 'P', i);
        bar(pct[i] / 100.0, 20);
        std::printf("  %5.1f%%\n", pct[i]);
    }

    const Ledger& m = b.mem;
    std::printf("\n  memory        %6.2f GB total\n", gb(m.total));
    std::printf("  free          %6.2f GB\n", gb(m.free));
    std::printf("  active        %6.2f GB\n", gb(m.active));
    std::printf("  inactive      %6.2f GB\n", gb(m.inactive));
    std::printf("  wired         %6.2f GB\n", gb(m.wired));
    std::printf("  compressed    %6.2f GB\n", gb(m.compressed));
    std::printf("  swap          %6.2f GB of %.2f\n", gb(m.swapUsed), gb(m.swapTotal));
    std::printf("  pressure      %d\n", m.pressure);

    const double wiredOfCeiling = static_cast<double>(m.wired) / static_cast<double>(b.gpuCeiling);
    std::printf("\n  gpu ceiling   %6.2f GB\n", gb(b.gpuCeiling));
    std::printf("  wired/ceiling ");
    bar(wiredOfCeiling, 30);
    std::printf("  %4.0f%%%s\n\n", 100.0 * wiredOfCeiling, wiredOfCeiling > 0.9 ? "  !! red band" : "");

    CHECK(b.cpu.cores.size() == topo.performance + topo.efficiency);
    CHECK(pct.size() == b.cpu.cores.size());
    CHECK(m.total > 0);
    CHECK(b.gpuCeiling > 0 && b.gpuCeiling < m.total);
    CHECK(m.wired < b.gpuCeiling); // if this fails, the machine is about to tell you itself

    return DONE();
}
