#include "PluginProcessor.h"

#include "hardtune/Scale.h"

HardTuneProcessor::HardTuneProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "HardTuneParameters", createParameterLayout())
{
    key = parameters.getRawParameterValue ("key");
    scale = parameters.getRawParameterValue ("scale");
    retuneMs = parameters.getRawParameterValue ("retune_ms");
    mix = parameters.getRawParameterValue ("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout HardTuneProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "key", 1 }, "Key",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));

    // Order must match hardtune::ScaleType
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "scale", 1 }, "Scale",
        juce::StringArray { "Chromatic", "Major", "Minor" }, 2));

    // 0 ms snaps instantly to the target note (the hard, robotic sound); larger values glide
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "retune_ms", 1 }, "Retune Speed",
        juce::NormalisableRange<float> { 0.0f, 200.0f, 1.0f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> { 0.0f, 1.0f }, 1.0f));

    return layout;
}

void HardTuneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Pitch detector and shifter buffers get allocated here, never in processBlock
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

bool HardTuneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void HardTuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    // Pass-through for now. Next steps: detect pitch (YIN), pick the target with
    // hardtune::nearestScaleNote, then shift the audio (PSOLA) by the difference.
}

juce::AudioProcessorEditor* HardTuneProcessor::createEditor()
{
    // JUCE's built-in editor shows a slider or menu for every parameter; a custom UI comes later
    return new juce::GenericAudioProcessorEditor (*this);
}

void HardTuneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HardTuneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HardTuneProcessor();
}
