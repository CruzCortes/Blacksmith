/*
 *  CPU load:
 *
 *  Author: Gonzalo Cruz Cortes
 *
 *  Description:
 *      The kernel counts, per core, the clock ticks spent in user, system,
 *      idle and nice state since boot. Those counters only ever grow, so a
 *      single reading says nothing about load. The difference between two
 *      readings is the load.
 *
 *      CoreTicks is one core's four counters. CpuSample is every core at
 *      one instant. CpuTopology is how many performance and efficiency
 *      cores the machine has. perCorePercent turns a pair of samples into
 *      a busy percentage per core.
 *
 *      Nothing here will call the operating system. That is what makes the
 *      arithmetic testable with hand-written numbers, including the 32-bit
 *      counter wrap a live machine reaches once every 497 days.
 */

#pragma once
#include <cstdint>
#include <vector>

namespace blacksmith::model {
    struct CoreTicks {
        std::uint32_t user      = 0;
        std::uint32_t system    = 0;
        std::uint32_t idle      = 0;
        std::uint32_t nice      = 0;
    };


    struct CpuSample {
        std::vector<CoreTicks> cores;
    };

    struct CpuTopology {
        unsigned performance    = 0;
        unsigned efficiency     = 0;
    };

    std::vector<double> perCorePercent(const CpuSample& before, const CpuSample& after);
}
