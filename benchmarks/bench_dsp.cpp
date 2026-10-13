// Performance benchmarks. Each audio benchmark reports how long one block takes; compare it with
// the real-time deadline (128 samples at 48 kHz = 2.67 ms) to get the share of one CPU core used.
// Run a Release build: build/default/benchmarks/Release/hardtune_benchmarks.exe

#include "hardtune/Engine.h"
#include "hardtune/RingBuffer.h"
#include "hardtune/Scale.h"
#include "hardtune/SpscQueue.h"

#include <benchmark/benchmark.h>

#include <cmath>
#include <vector>

namespace
{
std::vector<float> sineBlock (int length)
{
    std::vector<float> block (static_cast<std::size_t> (length));
    for (int i = 0; i < length; ++i)
        block[static_cast<std::size_t> (i)] = std::sin (0.05f * static_cast<float> (i));
    return block;
}
} // namespace

static void BM_NearestScaleNote (benchmark::State& state)
{
    double note = 60.0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize (hardtune::nearestScaleNote (note, 9, hardtune::ScaleType::NaturalMinor));
        note = note < 80.0 ? note + 0.01 : 60.0;
    }
}
BENCHMARK (BM_NearestScaleNote);

static void BM_RingBufferPushBlock (benchmark::State& state)
{
    const auto blockSize = static_cast<int> (state.range (0));
    const auto block = sineBlock (blockSize);
    hardtune::RingBuffer<float> buffer;
    buffer.prepare (8192);

    for (auto _ : state)
    {
        buffer.push (block.data(), block.size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed (state.iterations() * blockSize);
}
BENCHMARK (BM_RingBufferPushBlock)->Arg (128)->Arg (512);

static void BM_SpscQueuePushPop (benchmark::State& state)
{
    hardtune::SpscQueue<float, 1024> queue;
    float value = 0.0f;
    for (auto _ : state)
    {
        queue.tryPush (value);
        queue.tryPop (value);
        benchmark::DoNotOptimize (value);
    }
}
BENCHMARK (BM_SpscQueuePushPop);

// The whole engine on one stereo block, the number that matters for the CPU budget
static void BM_EngineProcessBlock (benchmark::State& state)
{
    const auto blockSize = static_cast<int> (state.range (0));
    auto left = sineBlock (blockSize);
    auto right = sineBlock (blockSize);
    float* channels[] = { left.data(), right.data() };

    hardtune::Engine engine;
    engine.prepare (48000.0, blockSize);

    for (auto _ : state)
    {
        engine.process (channels, 2, blockSize);
        benchmark::ClobberMemory();
    }

    // Share of one core: time per block divided by the block's duration at 48 kHz
    const auto blockSeconds = blockSize / 48000.0;
    state.counters["block_ms_budget"] = blockSeconds * 1000.0;
    state.SetItemsProcessed (state.iterations() * blockSize);
}
BENCHMARK (BM_EngineProcessBlock)->Arg (64)->Arg (128)->Arg (512);
