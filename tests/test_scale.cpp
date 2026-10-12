#include "hardtune/Scale.h"

#include <gtest/gtest.h>

#include <cmath>

using hardtune::ScaleType;

TEST (Frequency, A4IsMidi69)
{
    EXPECT_DOUBLE_EQ (hardtune::frequencyToMidi (440.0), 69.0);
    EXPECT_DOUBLE_EQ (hardtune::midiToFrequency (69.0), 440.0);
}

TEST (Frequency, MiddleC)
{
    EXPECT_NEAR (hardtune::frequencyToMidi (261.625565), 60.0, 1e-6);
}

TEST (Frequency, RoundTrip)
{
    for (double note = 20.0; note <= 100.0; note += 0.37)
        EXPECT_NEAR (hardtune::frequencyToMidi (hardtune::midiToFrequency (note)), note, 1e-9);
}

TEST (Snap, ChromaticRoundsToNearestSemitone)
{
    EXPECT_EQ (hardtune::nearestScaleNote (69.4, 0, ScaleType::Chromatic), 69);
    EXPECT_EQ (hardtune::nearestScaleNote (69.6, 0, ScaleType::Chromatic), 70);
}

TEST (Snap, TiesGoToTheLowerNote)
{
    EXPECT_EQ (hardtune::nearestScaleNote (69.5, 0, ScaleType::Chromatic), 69);
    // C#4 sits exactly between C4 and D4 in C major
    EXPECT_EQ (hardtune::nearestScaleNote (61.0, 0, ScaleType::Major), 60);
}

TEST (Snap, OutOfScaleNotesMoveToTheCloserNeighbor)
{
    // In C major, a slightly sharp C#4 is closer to D4
    EXPECT_EQ (hardtune::nearestScaleNote (61.2, 0, ScaleType::Major), 62);
    // In A minor, G#4 (68) is out of the scale; 68.3 is closer to A4 (69) than to G4 (67)
    EXPECT_EQ (hardtune::nearestScaleNote (68.3, 9, ScaleType::NaturalMinor), 69);
}

TEST (Scale, AMinorHasTheSameNotesAsCMajor)
{
    for (int note = 0; note < 128; ++note)
        EXPECT_EQ (hardtune::isInScale (note, 9, ScaleType::NaturalMinor),
                   hardtune::isInScale (note, 0, ScaleType::Major))
            << "note " << note;
}

TEST (Scale, WorksForNegativeNoteNumbers)
{
    EXPECT_TRUE (hardtune::isInScale (-12, 0, ScaleType::Major)); // a C
    EXPECT_FALSE (hardtune::isInScale (-11, 0, ScaleType::Major)); // a C#
    EXPECT_EQ (hardtune::nearestScaleNote (-0.4, 0, ScaleType::Major), 0);
}

TEST (Snap, ResultIsAlwaysInTheScaleAndClose)
{
    for (int root = 0; root < 12; ++root)
    {
        for (auto scale : { ScaleType::Chromatic, ScaleType::Major, ScaleType::NaturalMinor })
        {
            for (double note = 30.0; note <= 90.0; note += 0.13)
            {
                const auto snapped = hardtune::nearestScaleNote (note, root, scale);
                EXPECT_TRUE (hardtune::isInScale (snapped, root, scale));
                EXPECT_LE (std::abs (snapped - note), 1.0); // no scale here has a gap over 2 semitones
            }
        }
    }
}
