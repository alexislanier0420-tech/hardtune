#include "hardtune/Engine.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace
{
std::vector<float> testSignal (std::size_t length)
{
    std::vector<float> signal (length);
    for (std::size_t i = 0; i < length; ++i)
        signal[i] = 0.5f * std::sin (2.0f * 3.14159265f * 220.0f * static_cast<float> (i) / 48000.0f);
    return signal;
}

// Runs a stereo copy of `input` through a fresh engine using the given block sizes.
std::vector<float> runEngine (const std::vector<float>& input, const std::vector<int>& blockSizes, hardtune::Engine& engine)
{
    engine.prepare (48000.0, 1024);

    std::vector<float> left = input;
    std::vector<float> right = input;
    std::size_t position = 0;
    for (auto blockSize : blockSizes)
    {
        float* channels[] = { left.data() + position, right.data() + position };
        engine.process (channels, 2, blockSize);
        position += static_cast<std::size_t> (blockSize);
    }
    return left;
}
} // namespace

TEST (Engine, PassesAudioThroughUnchanged)
{
    const auto input = testSignal (4096);
    hardtune::Engine engine;
    EXPECT_EQ (runEngine (input, std::vector<int> (32, 128), engine), input);
}

TEST (Engine, RunsOneHopPer128Samples)
{
    const auto input = testSignal (128 * 10);
    hardtune::Engine engine;
    runEngine (input, std::vector<int> (10, 128), engine);
    EXPECT_EQ (engine.getHopCount(), 10u);
    EXPECT_EQ (engine.getLastHopSample(), 1280u);
}

// The same audio processed in random block sizes must give bit-identical output and the same
// hops. This guards the fixed-hop design as pitch detection and shifting are added.
TEST (Engine, OutputDoesNotDependOnBlockSize)
{
    constexpr std::size_t length = 48000;
    const auto input = testSignal (length);

    hardtune::Engine referenceEngine;
    const auto reference = runEngine (input, std::vector<int> (length / 128, 128), referenceEngine);

    std::mt19937 rng (42);
    std::uniform_int_distribution<int> size (1, 1024);
    for (int trial = 0; trial < 5; ++trial)
    {
        std::vector<int> blocks;
        std::size_t total = 0;
        while (total < length)
        {
            const auto block = std::min<std::size_t> (static_cast<std::size_t> (size (rng)), length - total);
            blocks.push_back (static_cast<int> (block));
            total += block;
        }

        hardtune::Engine engine;
        EXPECT_EQ (runEngine (input, blocks, engine), reference) << "trial " << trial;
        EXPECT_EQ (engine.getHopCount(), referenceEngine.getHopCount());
    }
}

TEST (Engine, IgnoresEmptyBlocks)
{
    hardtune::Engine engine;
    engine.prepare (48000.0, 512);
    float sample = 0.25f;
    float* channels[] = { &sample };
    engine.process (channels, 1, 0);
    engine.process (channels, 0, 1);
    EXPECT_EQ (engine.getHopCount(), 0u);
    EXPECT_EQ (sample, 0.25f);
}
