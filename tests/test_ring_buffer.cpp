#include "hardtune/RingBuffer.h"

#include <gtest/gtest.h>

#include <vector>

TEST (RingBuffer, CapacityRoundsUpToPowerOfTwo)
{
    hardtune::RingBuffer<float> buffer;
    buffer.prepare (1000);
    EXPECT_EQ (buffer.capacity(), 1024u);

    buffer.prepare (1024);
    EXPECT_EQ (buffer.capacity(), 1024u);
}

TEST (RingBuffer, StartsZeroed)
{
    hardtune::RingBuffer<float> buffer;
    buffer.prepare (8);
    for (std::size_t delay = 0; delay < buffer.capacity(); ++delay)
        EXPECT_EQ (buffer.fromNewest (delay), 0.0f);
}

TEST (RingBuffer, FromNewestCountsBackwards)
{
    hardtune::RingBuffer<float> buffer;
    buffer.prepare (8);
    for (int i = 1; i <= 5; ++i)
        buffer.push (static_cast<float> (i));

    EXPECT_EQ (buffer.fromNewest (0), 5.0f);
    EXPECT_EQ (buffer.fromNewest (1), 4.0f);
    EXPECT_EQ (buffer.fromNewest (4), 1.0f);
    EXPECT_EQ (buffer.fromNewest (5), 0.0f); // never written
}

TEST (RingBuffer, WrapsAroundAndKeepsTheNewestSamples)
{
    hardtune::RingBuffer<float> buffer;
    buffer.prepare (8);

    std::vector<float> input (21);
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<float> (i);
    buffer.push (input.data(), 7);
    buffer.push (input.data() + 7, 7);
    buffer.push (input.data() + 14, 7);

    std::vector<float> newest (8);
    buffer.copyNewest (newest.data(), newest.size());
    EXPECT_EQ (newest, (std::vector<float> { 13, 14, 15, 16, 17, 18, 19, 20 }));
}

TEST (RingBuffer, ResetClearsHistory)
{
    hardtune::RingBuffer<float> buffer;
    buffer.prepare (4);
    buffer.push (1.0f);
    buffer.reset();
    EXPECT_EQ (buffer.fromNewest (0), 0.0f);
}
