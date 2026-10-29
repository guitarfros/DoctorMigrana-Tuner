// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/** Coordinates of the artwork (Assets/pedal_base.jpg, 835 x 1210 px). */
namespace dmt::layout
{
    inline constexpr float artW = 835.0f, artH = 1210.0f;

    inline constexpr float knobRadius = 105.0f;
    inline const juce::Point<float> hzKnobCentre    { 172.0f, 313.0f };
    inline const juce::Point<float> centsKnobCentre { 673.0f, 313.0f };

    inline constexpr int   numLeds = 9;
    inline constexpr float ledX[numLeds] = { 118.5f, 194.0f, 271.0f, 348.5f, 424.5f, 500.5f, 577.5f, 655.0f, 731.5f };
    inline constexpr float ledY = 602.0f, ledRadius = 24.0f;

    inline const juce::Point<float> activeLedCentre { 170.7f, 923.3f };
    inline constexpr float activeLedRadius = 27.0f;

    inline const juce::Point<float> toggleCentre { 660.0f, 777.0f };
    inline const juce::Point<float> footSwitchCentre { 424.0f, 996.0f };
    inline constexpr float footSwitchRadius = 138.0f;
}

//==============================================================================
/** Rotary knob drawn from the artwork; the white pointer sweeps 270 degrees. */
class PedalKnob : public juce::Slider
{
public:
    explicit PedalKnob (const juce::Image& knobArt);

    /** Component is this many times larger than the knob so the pointer's tail fits. */
    static constexpr float boundsFactor = 1.32f;

    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;

private:
    float getKnobRadius() const noexcept;
    juce::Image art;
};

//==============================================================================
/** Mini toggle switch: lever ball on the left = MUTE, on the right = BYPASS. */
class LeverSwitch : public juce::ToggleButton
{
public:
    LeverSwitch();
    void tick();                                   // animates the lever (called from the editor timer)
    void paintButton (juce::Graphics&, bool, bool) override;
    bool hitTest (int x, int y) override;

private:
    float position = 0.0f;                         // 0 = MUTE (left), 1 = BYPASS (right)
};

//==============================================================================
/** Invisible button laid over the footswitch of the artwork. */
class FootSwitchButton : public juce::ToggleButton
{
public:
    FootSwitchButton();
    void paintButton (juce::Graphics&, bool, bool) override;
    bool hitTest (int x, int y) override;
};

//==============================================================================
class DoctorMigranaTunerEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit DoctorMigranaTunerEditor (DoctorMigranaTunerProcessor&);
    ~DoctorMigranaTunerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    float getScale() const noexcept { return (float) getWidth() / dmt::layout::artW; }

    DoctorMigranaTunerProcessor& proc;

    juce::TooltipWindow tooltipWindow { this, 700 };
    juce::Image baseImage, knobImage;

    PedalKnob        hzKnob, centsKnob;
    LeverSwitch      muteSwitch;
    FootSwitchButton footSwitch;

    juce::AudioProcessorValueTreeState::SliderAttachment hzAttachment, centsAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment muteAttachment, activeAttachment;

    std::atomic<float>* refParam    = nullptr;
    std::atomic<float>* tolParam    = nullptr;

    float ledLevel[dmt::layout::numLeds] {};
    float activeLedLevel = 0.0f;
    int   lastLedIndex   = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DoctorMigranaTunerEditor)
};
