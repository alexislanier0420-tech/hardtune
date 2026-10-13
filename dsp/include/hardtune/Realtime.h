#pragma once

#include <cstddef>

// HARDTUNE_NONBLOCKING marks functions that run on the real-time audio thread. With Clang 20+ and
// -fsanitize=realtime, RealtimeSanitizer aborts if such a function (or anything it calls) allocates
// memory, takes a lock or makes another blocking call. On other compilers it expands to nothing.
// It must appear on both the declaration and the definition.

#if defined(__clang__) && defined(__has_cpp_attribute)
    #if __has_cpp_attribute(clang::nonblocking)
        #define HARDTUNE_NONBLOCKING [[clang::nonblocking]]
    #endif
#endif

#ifndef HARDTUNE_NONBLOCKING
    #define HARDTUNE_NONBLOCKING
#endif

namespace hardtune
{

// Smallest power of two that is >= value (value must be > 0).
constexpr std::size_t nextPowerOfTwo (std::size_t value) noexcept
{
    std::size_t result = 1;
    while (result < value)
        result <<= 1;
    return result;
}

} // namespace hardtune
