// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DoctorMigranaTunerProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { dmt::ParamID::refHz, 1 }, "Reference (Hz)",
        NormalisableRange<float> (400.0f, 480.0f, 0.1f), 440.0f,
        AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { dmt::ParamID::centsTol, 1 }, "Tolerance (cents)",
        NormalisableRange<float> (1.0f, 25.0f, 1.0f), 1.0f,
        AudioParameterFloatAttributes().withLabel ("cents")));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { dmt::ParamID::active, 1 }, "Active", true));

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { dmt::ParamID::mute, 1 }, "Mute (off = Bypass)", true));

    return layout;
}

DoctorMigranaTunerProcessor::DoctorMigranaTunerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    activeParam = apvts.getRawParameterValue (dmt::ParamID::active);
    muteParam   = apvts.getRawParameterValue (dmt::ParamID::mute);
}

void DoctorMigranaTunerProcessor::prepareToPlay (double sampleRate, int)
{
    analyser.prepare (sampleRate);
    outputGain.reset (sampleRate, 0.015);
    outputGain.setCurrentAndTargetValue (1.0f);
}

void DoctorMigranaTunerProcessor::releaseResources()
{
    analyser.release();
}

bool DoctorMigranaTunerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void DoctorMigranaTunerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const bool active   = activeParam->load() > 0.5f;
    const bool muteMode = muteParam->load()   > 0.5f;

    // The tuner always listens to the *incoming* signal, before any muting.
    analyser.setEnabled (active);
    if (active)
        analyser.push (buffer.getArrayOfReadPointers(),
                       juce::jmin (buffer.getNumChannels(), getTotalNumInputChannels()),
                       numSamples);

    // Pedal on + MUTE  -> silence.   Pedal on + BYPASS, or pedal off -> signal passes.
    outputGain.setTargetValue ((active && muteMode) ? 0.0f : 1.0f);

    if (outputGain.isSmoothing())
    {
        float* const* w = buffer.getArrayOfWritePointers();
        const int numCh = buffer.getNumChannels();
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outputGain.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
                w[ch][i] *= g;
        }
    }
    else if (outputGain.getTargetValue() < 0.5f)
    {
        buffer.clear();
    }
}

juce::AudioProcessorEditor* DoctorMigranaTunerProcessor::createEditor()
{
    return new DoctorMigranaTunerEditor (*this);
}

void DoctorMigranaTunerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DoctorMigranaTunerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DoctorMigranaTunerProcessor();
}
