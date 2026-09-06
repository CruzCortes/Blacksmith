// test_gpu -- covers dev/00 task 0.10.
//
// What it proves:
//   * there is a Metal device and it reports a working-set ceiling
//   * the ceiling is below physical memory and above half of it
//     (Apple sets it near 75% on unified-memory Macs)
//   * asking twice gives the same answer and leaves nothing behind
//     (run under ASan; a leaked device is silent, a double release is not)

#include "check.hpp"
#include "gpu.hpp"

#include <cstdio>

#include <sys/sysctl.h>

static std::uint64_t memsize() {
    std::uint64_t v = 0;
    std::size_t len = sizeof(v);
    sysctlbyname("hw.memsize", &v, &len, nullptr, 0);
    return v;
}

int main() {
    using namespace blacksmith;
    using check::gb;

    const std::uint64_t total = memsize();
    const std::uint64_t ceiling = gpuWorkingSetLimit();

    std::printf("  gpu working-set ceiling  %6.2f GB  (%.0f%% of %.0f GB)\n\n", gb(ceiling),
                100.0 * static_cast<double>(ceiling) / static_cast<double>(total), gb(total));

    CHECK(ceiling > 0);
    CHECK(ceiling < total);
    CHECK(ceiling > total / 2);

    for (int i = 0; i < 100; ++i) CHECK(gpuWorkingSetLimit() == ceiling);

    return DONE();
}
