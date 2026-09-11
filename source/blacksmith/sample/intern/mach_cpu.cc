/*
 *  Core counts and per-core tick counters.
 *
 *  Author: Gonzalo Cruz Cortes
 *
 *  Two orderings to keep straight. sysctl numbers performance levels by
 *  speed, so hw.perflevel0 is Performance and hw.perflevel1 is Efficiency.
 *  The per-core array Mach returns runs the other way: efficiency cores
 *  occupy the low indices. On an M2 Max that is 8 performance and 4
 *  efficiency, indices 0 to 3 being the efficiency cluster.
 */

 #include "sample/sample.hh"
 #include <sys/sysctl.h>
 #include <cstddef>
 #include <cstdint>

 namespace blacksmith::sample {
    
    model::CpuTopology cpuTopology() {
        model::CpuTopology t;

        std::uint32_t value = 0;
        std::size_t size = sizeof(value);

        if (sysctlbyname( "hw.perflevel0.logicalcpu", &value, &size, nullptr, 0 ) == 0 ) {
            t.performance = value;
        }

        value = 0;
        size = sizeof(value);

        if ( sysctlbyname( "hw.perflevel1.logicalcpu", &value, &size, nullptr, 0 ) == 0 ) {
            t.efficiency = value;
        }

        return t;
    }

 } // namespace blacksmith::sample