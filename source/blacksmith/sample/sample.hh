/*
 *  Everything Blacksmith asks the operating system.
 *  
 *  Author: Gonzalo Cruz Cortes
 * 
 *  One header for the whole module. Callers get plain model types back and
 *  never learn whether a number came from Mach, sysctl, Metal or IOKit.
 *  Changing where a number comes from touches one file under intern/ and
 *  nothing else in the program. 
 */

#pragma once
#include "model/cpu.hh"

namespace blacksmith::sample {

    model::CpuTopology cpuTopology();
    
} // namespace blacksmith::sample