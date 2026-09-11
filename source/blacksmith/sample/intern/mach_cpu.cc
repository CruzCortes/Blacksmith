/*
 *  Core counts and per-core tick counters.
 *
 *  Author: Gonzalo Cruz Cortes
 *
 *  Apple orders core clusters fastest first, so level 0 is always the quick
 *  one. The names are not fixed: M1 through M4 call the two levels
 *  Performance and Efficiency, M5 calls level 0 Super. Read hw.nperflevels
 *  and index off it instead of spelling a digit into the key.
 *
 *  The per-core array Mach returns runs the other way round: efficiency
 *  cores occupy the low indices.
 */

 #include "sample/sample.hh"
 #include <sys/sysctl.h>
 #include <cstddef>
 #include <cstdint>
 #include <cstdio>

 namespace blacksmith::sample {
    
    model::CpuTopology cpuTopology() {
        model::CpuTopology t;

        std::uint32_t levels = 0;
        std::size_t size = sizeof(levels);

        if ( sysctlbyname( "hw.nperflevels", &levels, &size, nullptr, 0 ) != 0 ) {
            return t;
        }

        for ( std::uint32_t i = 0; i < levels; ++i ) {
            char key[64];
            std::snprintf( key, sizeof(key), "hw.perflevel%u.logicalcpu", i );

            std::uint32_t count = 0;
            size = sizeof(count);
            if ( sysctlbyname( key, &count, &size, nullptr, 0 ) != 0 ) continue;

            if ( i == 0 ) t.performance = count;
            else if ( i == 1 ) t.efficiency = count;
        }

        return t;
    }

 } // namespace blacksmith::sample