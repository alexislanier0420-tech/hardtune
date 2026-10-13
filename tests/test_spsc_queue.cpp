#include "hardtune/SpscQueue.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <thread>

TEST (SpscQueue, PopFromEmptyFails)
{
    hardtune::SpscQueue<int, 4> queue;
    int value = -1;
    EXPECT_FALSE (queue.tryPop (value));
    EXPECT_EQ (value, -1);
}

TEST (SpscQueue, PreservesOrder)
{
    hardtune::SpscQueue<int, 8> queue;
    for (int i = 0; i < 5; ++i)
        ASSERT_TRUE (queue.tryPush (i));

    for (int i = 0; i < 5; ++i)
    {
        int value = -1;
        ASSERT_TRUE (queue.tryPop (value));
        EXPECT_EQ (value, i);
    }
}

TEST (SpscQueue, PushToFullFailsWithoutOverwriting)
{
    hardtune::SpscQueue<int, 4> queue;
    for (int i = 0; i < 4; ++i)
        ASSERT_TRUE (queue.tryPush (i));
    EXPECT_FALSE (queue.tryPush (99));
    EXPECT_EQ (queue.size(), 4u);

    int value = -1;
    ASSERT_TRUE (queue.tryPop (value));
    EXPECT_EQ (value, 0);
    EXPECT_TRUE (queue.tryPush (4)); // the freed slot can be reused
}

TEST (SpscQueue, WrapsAroundManyTimes)
{
    hardtune::SpscQueue<int, 4> queue;
    for (int i = 0; i < 1000; ++i)
    {
        ASSERT_TRUE (queue.tryPush (i));
        int value = -1;
        ASSERT_TRUE (queue.tryPop (value));
        EXPECT_EQ (value, i);
    }
}

// One thread pushes 0, 1, 2, ... while another pops. Every value must arrive exactly once and in
// order. Run under ThreadSanitizer in CI to catch data races the assertions can't see.
TEST (SpscQueue, TwoThreadStressTestDeliversEverythingInOrder)
{
    constexpr std::uint64_t itemCount = 1'000'000;
    hardtune::SpscQueue<std::uint64_t, 64> queue;

    std::thread producer ([&] {
        for (std::uint64_t i = 0; i < itemCount;)
            if (queue.tryPush (i))
                ++i;
            else
                std::this_thread::yield();
    });

    std::uint64_t expected = 0;
    bool inOrder = true;
    while (expected < itemCount)
    {
        std::uint64_t value = 0;
        if (queue.tryPop (value))
        {
            inOrder = inOrder && (value == expected);
            ++expected;
        }
        else
        {
            std::this_thread::yield();
        }
    }

    producer.join();
    EXPECT_TRUE (inOrder);
    EXPECT_EQ (queue.size(), 0u);
}
