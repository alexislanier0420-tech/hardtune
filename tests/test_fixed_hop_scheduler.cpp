#include "hardtune/FixedHopScheduler.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

namespace
{
// Feeds `blockSizes` through a scheduler and returns the absolute sample index of every hop.
std::vector<int> hopPositions (const std::vector<int>& blockSizes, int hopSize)
{
    hardtune::FixedHopScheduler scheduler;
    scheduler.prepare (hopSize);

    // Reserve up front: push_back must not allocate inside process(), which RealtimeSanitizer checks
    int total = 0;
    for (auto blockSize : blockSizes)
        total += blockSize;
    std::vector<int> hops;
    hops.reserve (static_cast<std::size_t> (total / hopSize + 1));

    int position = 0;
    for (auto blockSize : blockSizes)
    {
        int covered = 0;
        scheduler.process (
            blockSize,
            [&] (int start, int count) {
                EXPECT_EQ (start, covered); // pieces are contiguous and in order
                covered += count;
                position += count;
            },
            [&] { hops.push_back (position); });
        EXPECT_EQ (covered, blockSize); // every sample of the block is covered exactly once
    }
    return hops;
}
} // namespace

TEST (FixedHopScheduler, HopsEveryHopSizeSamples)
{
    EXPECT_EQ (hopPositions ({ 128, 128, 128 }, 128), (std::vector<int> { 128, 256, 384 }));
}

TEST (FixedHopScheduler, SplitsBlocksThatCrossAHopBoundary)
{
    EXPECT_EQ (hopPositions ({ 100, 100, 100 }, 128), (std::vector<int> { 128, 256 }));
}

TEST (FixedHopScheduler, HandlesBlocksLargerThanAHop)
{
    EXPECT_EQ (hopPositions ({ 512 }, 128), (std::vector<int> { 128, 256, 384, 512 }));
}

TEST (FixedHopScheduler, HandlesEmptyAndSingleSampleBlocks)
{
    std::vector<int> blocks (300, 1);
    blocks.insert (blocks.begin() + 50, 0);
    EXPECT_EQ (hopPositions (blocks, 128), (std::vector<int> { 128, 256 }));
}

TEST (FixedHopScheduler, HopPositionsDoNotDependOnBlockSizes)
{
    const auto reference = hopPositions (std::vector<int> (100, 128), 128);

    std::mt19937 rng (1234);
    std::uniform_int_distribution<int> size (1, 1024);
    for (int trial = 0; trial < 20; ++trial)
    {
        std::vector<int> blocks;
        int total = 0;
        while (total < 100 * 128)
        {
            const auto block = std::min (size (rng), 100 * 128 - total);
            blocks.push_back (block);
            total += block;
        }
        EXPECT_EQ (hopPositions (blocks, 128), reference) << "trial " << trial;
    }
}
