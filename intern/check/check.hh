// check -- a test harness in one header. No framework: a counter and a macro.
//
//     CHECK(expr);          records pass or fail, prints the failing line
//     return DONE();        prints the tally, returns 1 if anything failed
//
// Lives in intern/ because nothing in it is Blacksmith-specific. Tests print
// what they measured before checking it, so a failure comes with the number
// that caused it.
#pragma once

#include <cstdint>
#include <cstdio>

namespace check {
inline int passed = 0;
inline int failed = 0;

// bytes -> GB as a double, for printing only. Never store a GB anywhere.
inline double gb(std::uint64_t bytes) { return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0); }
} // namespace check

#define CHECK(expr)                                                                          \
    do {                                                                                     \
        if (expr) {                                                                          \
            ++check::passed;                                                                 \
        } else {                                                                             \
            ++check::failed;                                                                 \
            std::printf("  FAIL  %s:%d   %s\n", __FILE__, __LINE__, #expr);                   \
        }                                                                                    \
    } while (0)

#define DONE()                                                                               \
    (std::printf("  %d passed, %d failed\n", check::passed, check::failed), check::failed ? 1 : 0)
