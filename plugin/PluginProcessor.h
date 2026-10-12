#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// The plugin wrapper around the DSP code. For now it passes audio through unchanged and exposes
// the parameters the pitch correction will use; the processing itself lives in dsp/.
class HardTuneProcessor final : public juce::AudioProcessor
{
public:
    HardTuneProcessor();
    ~HardTuneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState parameters;

    // Cached parameter pointers, read on the audio thread without locking
    std::atomic<float>* key = nullptr;
    std::atomic<float>* scale = nullptr;
    std::atomic<float>* retuneMs = nullptr;
    std::atomic<float>* mix = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardTuneProcessor)
};
