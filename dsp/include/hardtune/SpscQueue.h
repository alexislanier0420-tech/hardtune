#pragma once

#include "hardtune/Realtime.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace hardtune
{

// A lock-free queue for exactly one producer thread and one consumer thread, used to send data
// from the audio thread to the UI without ever blocking the audio thread.
//
// How it stays correct without locks:
// - Only the producer writes `tail` and only the consumer writes `head`.
// - The producer writes the item, then publishes it with a release store of `tail`. The consumer
//   reads `tail` with an acquire load, which guarantees it also sees the item's contents.
// - The same pairing in the other direction (release store of `head`, acquire load by the
//   producer) guarantees a slot is only reused after the consumer has finished reading it.
// - The indices only ever increase; masking by Capacity - 1 maps them to slots, and unsigned
//   wrap-around keeps `tail - head` correct even after 2^64 items.
template <typename T, std::size_t Capacity>
class SpscQueue
{
    static_assert (Capacity >= 2 && (Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert (std::is_trivially_copyable_v<T>, "Items are copied by value; keep them trivially copyable");

public:
    // Producer only. Returns false (and drops the item) if the queue is full; never blocks.
    bool tryPush (const T& item) noexcept HARDTUNE_NONBLOCKING
    {
        const auto tail = tailIndex.load (std::memory_order_relaxed);
        const auto head = headIndex.load (std::memory_order_acquire);

        if (tail - head == Capacity)
            return false;

        slots[tail & (Capacity - 1)] = item;
        tailIndex.store (tail + 1, std::memory_order_release);
        return true;
    }

    // Consumer only. Returns false if the queue is empty.
    bool tryPop (T& item) noexcept HARDTUNE_NONBLOCKING
    {
        const auto head = headIndex.load (std::memory_order_relaxed);
        const auto tail = tailIndex.load (std::memory_order_acquire);

        if (head == tail)
            return false;

        item = slots[head & (Capacity - 1)];
        headIndex.store (head + 1, std::memory_order_release);
        return true;
    }

    // Approximate when called while the other thread is active.
    std::size_t size() const noexcept
    {
        return tailIndex.load (std::memory_order_acquire) - headIndex.load (std::memory_order_acquire);
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    // Separate cache lines so the two threads don't slow each other down by sharing one (false sharing).
    static constexpr std::size_t cacheLine = 64;

    alignas (cacheLine) std::atomic<std::size_t> headIndex { 0 };
    alignas (cacheLine) std::atomic<std::size_t> tailIndex { 0 };
    alignas (cacheLine) std::array<T, Capacity> slots {};
};

} // namespace hardtune
