#pragma once

#include "hardtune/FixedHopScheduler.h"
#include "hardtune/Realtime.h"
#include "hardtune/RingBuffer.h"

#include <cstdint>

namespace hardtune
{

// The whole pitch-correction pipeline, independent of any plugin framework. The plugin, the Python
// bindings and the browser demo all drive this class.
//
// Current stage: audio passes through unchanged while the input is recorded into a history buffer
// and analysis hops are scheduled. Pitch detection and shifting plug into onHop() and the
// per-sample path in later milestones.
class Engine
{
public:
    static constexpr int hopSize = 128;

    // Not real-time safe: allocates. Call before audio starts.
    void prepare (double sampleRate, int maximumBlockSize);
    void reset() noexcept;

    // Processes audio in place. channels[c][i] is sample i of channel c. Channel 0 is analysed;
    // the same correction will be applied to every channel.
    void process (float* const* channels, int numChannels, int numSamples) noexcept HARDTUNE_NONBLOCKING;

    double getSampleRate() const noexcept { return sampleRate; }

    // Number of analysis hops run since prepare() or reset(); used by tests.
    std::uint64_t getHopCount() const noexcept { return hopCount; }

    // Absolute input sample index at which the most recent hop ran; used by tests.
    std::uint64_t getLastHopSample() const noexcept { return lastHopSample; }

private:
    void onHop() noexcept HARDTUNE_NONBLOCKING;

    double sampleRate = 48000.0;
    FixedHopScheduler scheduler;
    RingBuffer<float> inputHistory;

    std::uint64_t samplesReceived = 0;
    std::uint64_t hopCount = 0;
    std::uint64_t lastHopSample = 0;
};

} // namespace hardtune
