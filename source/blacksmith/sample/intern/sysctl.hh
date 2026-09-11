/*
 *  One sysctl read, size-checked.  
 * 
 *  Author: Gonzalo Cruz Cortes
 *
 *  Private to the sample module. sysctlbyname reports how many bytes it
 *  wrote and will succeed after writing fewer than you offered, leaving
 *  the rest of your variable untouched. sysctlRead returns true only when
 *  the count the kernel reports equals the size that was asked for.
 */

#pragma once
#include <cstddef>

namespace blacksmith::sample {

    bool sysctlRead( const char* name, void* out, std::size_t size );

} // namespace blacksmith::sample