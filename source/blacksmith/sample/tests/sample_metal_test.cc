// sample::gpuWorkingSetLimit against the default Metal device.
//
// Checks:
//   * there is a Metal device and it reports a working-set ceiling
//   * the ceiling is below physical memory and above half of it
//     (Apple sets it near 75% on unified-memory Macs)
//   * asking a hundred times gives the same answer and leaves nothing behind
//     (a leaked device is silent under ASan; a double release is not)

#include "sample/intern/sysctl.hh"
#include "sample/sample.hh"

#include "check/check.hh"

#include <cstdint>
#include <cstdio>

int main() {
    using namespace blacksmith;
    using check::gb;

    std::uint64_t total = 0;
    sample::sysctlRead("hw.memsize", &total, sizeof(total));
    const std::uint64_t ceiling = sample::gpuWorkingSetLimit();

    std::printf("  gpu working-set ceiling  %6.2f GB  (%.0f%% of %.0f GB)\n\n", gb(ceiling),
                100.0 * static_cast<double>(ceiling) / static_cast<double>(total), gb(total));

    CHECK(ceiling > 0);
    CHECK(ceiling < total);
    CHECK(ceiling > total / 2);

    for (int i = 0; i < 100; ++i) CHECK(sample::gpuWorkingSetLimit() == ceiling);

    return DONE();
}
