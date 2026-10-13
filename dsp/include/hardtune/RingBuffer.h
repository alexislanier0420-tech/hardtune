#pragma once

#include "hardtune/Realtime.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace hardtune
{

// Keeps the most recent samples of a signal. Memory is allocated once in prepare(); after that,
// writing and reading never allocate. The capacity is a power of two, so wrapping an index is a
// bit mask instead of a division.
template <typename T>
class RingBuffer
{
public:
    // Not real-time safe: allocates. Call before audio starts (e.g. in prepareToPlay).
    void prepare (std::size_t minimumCapacity)
    {
        data.assign (nextPowerOfTwo (minimumCapacity), T {});
        mask = data.size() - 1;
        writeIndex = 0;
    }

    void reset() noexcept
    {
        std::fill (data.begin(), data.end(), T {});
        writeIndex = 0;
    }

    std::size_t capacity() const noexcept { return data.size(); }

    void push (T value) noexcept HARDTUNE_NONBLOCKING
    {
        data[writeIndex] = value;
        writeIndex = (writeIndex + 1) & mask;
    }

    void push (const T* values, std::size_t count) noexcept HARDTUNE_NONBLOCKING
    {
        assert (count <= data.size());
        for (std::size_t i = 0; i < count; ++i)
        {
            data[writeIndex] = values[i];
            writeIndex = (writeIndex + 1) & mask;
        }
    }

    // The sample written `delay` samples ago: 0 is the newest, capacity() - 1 the oldest kept.
    T fromNewest (std::size_t delay) const noexcept HARDTUNE_NONBLOCKING
    {
        assert (delay < data.size());
        return data[(writeIndex - 1 - delay) & mask];
    }

    // Copies the newest `count` samples into dest, oldest first, ready for analysis.
    void copyNewest (T* dest, std::size_t count) const noexcept HARDTUNE_NONBLOCKING
    {
        assert (count <= data.size());
        auto readIndex = (writeIndex - count) & mask;
        for (std::size_t i = 0; i < count; ++i)
        {
            dest[i] = data[readIndex];
            readIndex = (readIndex + 1) & mask;
        }
    }

private:
    std::vector<T> data;
    std::size_t mask = 0;
    std::size_t writeIndex = 0;
};

} // namespace hardtune
