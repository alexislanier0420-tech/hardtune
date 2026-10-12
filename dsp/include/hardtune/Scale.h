#pragma once

// Converting between frequency and MIDI note numbers, and snapping a detected pitch to the
// nearest note of a musical scale. These run on the audio thread, so none of them allocate.

namespace hardtune
{

enum class ScaleType
{
    Chromatic,
    Major,
    NaturalMinor
};

// Fractional MIDI note number for a frequency in Hz (A4 = 440 Hz = 69). Requires hz > 0.
double frequencyToMidi (double hz) noexcept;

// Frequency in Hz for a (possibly fractional) MIDI note number.
double midiToFrequency (double midiNote) noexcept;

// True if the note belongs to the scale. root is the key's pitch class (0 = C, 1 = C#, ..., 11 = B).
bool isInScale (int midiNote, int root, ScaleType scale) noexcept;

// The scale note closest to a fractional MIDI note. When two scale notes are equally close,
// the lower one wins, so the result is always deterministic.
int nearestScaleNote (double midiNote, int root, ScaleType scale) noexcept;

} // namespace hardtune
