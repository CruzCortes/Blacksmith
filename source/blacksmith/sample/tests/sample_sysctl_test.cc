// sample_sysctl_test -- covers dev/00 tasks 0.2 and 0.9.
//
// What it proves:
//   * cpuTopology() agrees with the number of online processors
//   * sysctlRead() says true only when the kernel filled exactly the bytes
//     you offered: wrong size and unknown name both come back false
//
// sysctlRead is private to the sample module (sample/intern/). A module's
// own tests may include its intern/ headers. Nothing else may.

#include "sample/intern/sysctl.hh"
#include "sample/sample.hh"

#include "check/check.hh"

#include <cstdint>
#include <cstdio>

#include <unistd.h>

int main() {
    using namespace blacksmith;

    const model::CpuTopology topo = sample::cpuTopology();
    const auto online = static_cast<unsigned>(sysconf(_SC_NPROCESSORS_ONLN));
    std::printf("  topology   P=%u  E=%u   online=%u\n", topo.performance, topo.efficiency, online);
    CHECK(topo.performance > 0);
    CHECK(topo.efficiency > 0);
    CHECK(topo.performance + topo.efficiency == online);

    std::uint64_t memsize = 0;
    CHECK(sample::sysctlRead("hw.memsize", &memsize, sizeof(memsize)));
    CHECK(memsize > 0);
    std::printf("  hw.memsize %.2f GB\n", check::gb(memsize));

    std::uint32_t tooSmall = 0;
    CHECK(!sample::sysctlRead("hw.memsize", &tooSmall, sizeof(tooSmall)));

    std::uint64_t junk = 0;
    CHECK(!sample::sysctlRead("hw.blacksmith_no_such_key", &junk, sizeof(junk)));

    return DONE();
}
