// sample::memory against the live machine.
//
// Checks:
//   * Ledger.total is hw.memsize, straight from the kernel
//   * the five page-backed rows sum to at most total, and to most of it
//   * every row is in BYTES. A forgotten page multiply shows up as a wired
//     figure under 1 GB, which this machine has not seen since boot
//   * swap and pressure are in range

#include "model/memory.hh"
#include "sample/intern/sysctl.hh"
#include "sample/sample.hh"

#include "check/check.hh"

#include <cstdint>
#include <cstdio>

int main() {
    using namespace blacksmith;
    using check::gb;

    model::Ledger l;
    sample::memory(l);

    std::printf("  total       %6.2f GB\n", gb(l.total));
    std::printf("  free        %6.2f GB\n", gb(l.free));
    std::printf("  active      %6.2f GB\n", gb(l.active));
    std::printf("  inactive    %6.2f GB\n", gb(l.inactive));
    std::printf("  wired       %6.2f GB\n", gb(l.wired));
    std::printf("  compressed  %6.2f GB\n", gb(l.compressed));
    std::printf("  swap        %6.2f / %.2f GB\n", gb(l.swapUsed), gb(l.swapTotal));
    std::printf("  pressure    %d\n\n", l.pressure);

    std::uint64_t memsize = 0;
    sample::sysctlRead("hw.memsize", &memsize, sizeof(memsize));
    CHECK(l.total == memsize);

    const std::uint64_t accounted = l.free + l.active + l.inactive + l.wired + l.compressed;
    std::printf("  accounted   %6.2f GB  (%.0f%% of total)\n\n", gb(accounted),
                100.0 * static_cast<double>(accounted) / static_cast<double>(l.total));
    CHECK(accounted <= l.total);
    CHECK(accounted >= l.total / 10 * 8); // speculative and purgeable pages are the gap

    // Bytes, not pages. A page count for wired would be ~400k, well under 1 GB.
    CHECK(l.wired > (1ull << 30));
    CHECK(l.free % 4096 == 0);

    CHECK(l.swapUsed <= l.swapTotal);
    CHECK(l.pressure == 1 || l.pressure == 2 || l.pressure == 4);

    return DONE();
}
