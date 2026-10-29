// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "TunerAnalyser.h"

namespace dmt::ParamID
{
    inline constexpr const char* refHz    = "refHz";      // reference pitch of A4, 400..480 Hz
    inline constexpr const char* centsTol = "centsTol";   // "in tune" tolerance, 1..25 cents
    inline constexpr const char* active   = "active";     // footswitch (tuner on/off)
    inline constexpr const char* mute     = "mute";       // true = MUTE, false = BYPASS (while active)
}

class DoctorMigranaTunerProcessor : public juce::AudioProcessor
{
public:
    DoctorMigranaTunerProcessor();
    ~DoctorMigranaTunerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                        { return true; }

    const juce::String getName() const override            { return JucePlugin_Name; }
    bool acceptsMidi() const override                      { return false; }
    bool producesMidi() const override                     { return false; }
    bool isMidiEffect() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0.0; }

    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const juce::String getProgramName (int) override       { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    TunerAnalyser& getAnalyser() noexcept                  { return analyser; }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    TunerAnalyser analyser;
    std::atomic<float>* activeParam = nullptr;
    std::atomic<float>* muteParam   = nullptr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoctorMigranaTunerProcessor)
};
