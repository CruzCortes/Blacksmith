/*
 *  Per-core busy percentage from two tick samples.
 *
 *  Author: Gonzalo Cruz Cortes
 *
 *  Two things to get wrong here. Ticks are 32 bits and roll over, so the
 *  subtraction has to happen in 32 bits as well; widen either side first
 *  and a rollover reads as a delta near four billion.
 *
 *  Second, a core that did not move between samples has a total of zero
 *  ticks. Dividing by that gives NaN, which reaches the terminal as a
 *  blank bar where a zero belongs.
 */

#include "model/cpu.hh"

namespace blacksmith::model {

    std::vector<double> perCorePercent( const CpuSample& before, const CpuSample& after ) {
        return {};
    }

} // namespace blacksmith::model