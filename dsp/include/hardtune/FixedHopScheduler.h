#pragma once

#include "hardtune/Realtime.h"

#include <algorithm>
#include <cassert>

namespace hardtune
{

// Splits host blocks of any size so that analysis runs every `hopSize` samples of input, counted
// from the start of the stream. Hops land on the same samples whatever block sizes the host uses,
// so the plugin's output doesn't depend on the host's buffer size.
//
// For each host block, process() calls:
//   onSamples (start, count)  for each piece of the block between hop boundaries, in order
//   onHop()                   each time a full hop of input has been received
class FixedHopScheduler
{
public:
    void prepare (int newHopSize) noexcept
    {
        assert (newHopSize > 0);
        hopSize = newHopSize;
        reset();
    }

    void reset() noexcept { samplesUntilHop = hopSize; }

    int getHopSize() const noexcept { return hopSize; }

    template <typename OnSamples, typename OnHop>
    void process (int numSamples, OnSamples&& onSamples, OnHop&& onHop) noexcept HARDTUNE_NONBLOCKING
    {
        int start = 0;

        while (start < numSamples)
        {
            const auto count = std::min (numSamples - start, samplesUntilHop);
            onSamples (start, count);
            start += count;
            samplesUntilHop -= count;

            if (samplesUntilHop == 0)
            {
                onHop();
                samplesUntilHop = hopSize;
            }
        }
    }

private:
    int hopSize = 128;
    int samplesUntilHop = 128;
};

} // namespace hardtune
