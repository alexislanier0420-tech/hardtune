#include "hardtune/Scale.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace hardtune
{

namespace
{
// Which of the 12 semitones above the root belong to each scale.
constexpr std::array<bool, 12> majorSteps { true, false, true, false, true, true, false, true, false, true, false, true };
constexpr std::array<bool, 12> minorSteps { true, false, true, true, false, true, false, true, true, false, true, false };

// Pitch class 0-11, correct for negative note numbers too.
constexpr int pitchClass (int note) noexcept
{
    return ((note % 12) + 12) % 12;
}
} // namespace

double frequencyToMidi (double hz) noexcept
{
    return 69.0 + 12.0 * std::log2 (hz / 440.0);
}

double midiToFrequency (double midiNote) noexcept
{
    return 440.0 * std::exp2 ((midiNote - 69.0) / 12.0);
}

bool isInScale (int midiNote, int root, ScaleType scale) noexcept
{
    const auto step = static_cast<std::size_t> (pitchClass (midiNote - root));

    switch (scale)
    {
        case ScaleType::Chromatic:    return true;
        case ScaleType::Major:        return majorSteps[step];
        case ScaleType::NaturalMinor: return minorSteps[step];
    }

    return true;
}

int nearestScaleNote (double midiNote, int root, ScaleType scale) noexcept
{
    // Every supported scale has a note within 2 semitones, so searching 6 below to 7 above the
    // floor is more than enough. Searching upward with a strict < keeps the lower note on ties.
    const auto base = static_cast<int> (std::floor (midiNote));
    auto best = base;
    auto bestDistance = std::numeric_limits<double>::max();

    for (auto candidate = base - 6; candidate <= base + 7; ++candidate)
    {
        if (! isInScale (candidate, root, scale))
            continue;

        const auto distance = std::abs (static_cast<double> (candidate) - midiNote);

        if (distance < bestDistance)
        {
            best = candidate;
            bestDistance = distance;
        }
    }

    return best;
}

} // namespace hardtune
