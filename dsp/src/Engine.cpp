#include "hardtune/Engine.h"

#include <cstddef>

namespace hardtune
{

namespace
{
// Longest pitch period YIN will need to look back over two periods of: 2 x (48 kHz / 60 Hz) = 1,600
// samples at 48 kHz. Sized generously so higher sample rates still fit.
constexpr double historySeconds = 0.1;
} // namespace

void Engine::prepare (double newSampleRate, int maximumBlockSize)
{
    sampleRate = newSampleRate;
    scheduler.prepare (hopSize);

    const auto historySamples = static_cast<std::size_t> (historySeconds * sampleRate) + static_cast<std::size_t> (maximumBlockSize);
    inputHistory.prepare (historySamples);

    reset();
}

void Engine::reset() noexcept
{
    scheduler.reset();
    inputHistory.reset();
    samplesReceived = 0;
    hopCount = 0;
    lastHopSample = 0;
}

void Engine::process (float* const* channels, int numChannels, int numSamples) noexcept HARDTUNE_NONBLOCKING
{
    if (numChannels <= 0 || numSamples <= 0)
        return;

    const float* analysisChannel = channels[0];

    scheduler.process (
        numSamples,
        [&] (int start, int count)
        {
            inputHistory.push (analysisChannel + start, static_cast<std::size_t> (count));
            samplesReceived += static_cast<std::uint64_t> (count);
        },
        [&] { onHop(); });

    // Pass-through: the audio in `channels` is left unchanged for now.
}

void Engine::onHop() noexcept HARDTUNE_NONBLOCKING
{
    ++hopCount;
    lastHopSample = samplesReceived;
}

} // namespace hardtune
