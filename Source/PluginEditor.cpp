// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

using namespace juce;
namespace L = dmt::layout;

//==============================================================================
// helpers

static void strokeRound (Graphics& g, Point<float> a, Point<float> b, float thickness)
{
    Path p;
    p.startNewSubPath (a);
    p.lineTo (b);
    g.strokePath (p, PathStrokeType (thickness, PathStrokeType::curved, PathStrokeType::rounded));
}

static bool insideCircle (Component& c, int x, int y, float radiusFraction)
{
    const auto centre = c.getLocalBounds().toFloat().getCentre();
    const float r = (float) jmin (c.getWidth(), c.getHeight()) * 0.5f * radiusFraction;
    return centre.getDistanceFrom ({ (float) x, (float) y }) <= r;
}

/** Paints a lit LED (glow + lens + specular) in artwork coordinates. */
static void drawLitLed (Graphics& g, Point<float> c, float r, Colour colour, float level)
{
    if (level < 0.02f)
        return;

    {   // halo
        ColourGradient halo (colour.withAlpha (0.50f * level), c,
                             colour.withAlpha (0.0f), c.translated (r * 2.2f, 0.0f), true);
        g.setGradientFill (halo);
        g.fillEllipse (Rectangle<float> (r * 4.4f, r * 4.4f).withCentre (c));
    }
    {   // lens
        ColourGradient lens (Colours::white.interpolatedWith (colour, 0.30f).withAlpha (level), c,
                             colour.darker (0.35f).withAlpha (0.95f * level), c.translated (r * 0.98f, 0.0f), true);
        g.setGradientFill (lens);
        g.fillEllipse (Rectangle<float> (r * 1.96f, r * 1.96f).withCentre (c));
    }
    {   // specular glint
        g.setColour (Colours::white.withAlpha (0.35f * level));
        g.fillEllipse (Rectangle<float> (r * 0.55f, r * 0.38f).withCentre (c.translated (-r * 0.38f, -r * 0.50f)));
    }
}

static Colour ledColour (int i)
{
    static const Colour colours[L::numLeds] = {
        Colour (0xffff3a2e),   // far flat
        Colour (0xffffa31f),
        Colour (0xffeee03a),
        Colour (0xffa4e63a),
        Colour (0xff3dff6a),   // in tune
        Colour (0xffa4e63a),
        Colour (0xffeee03a),
        Colour (0xffffa31f),
        Colour (0xffff3a2e)    // far sharp
    };
    return colours[i];
}

//==============================================================================
// PedalKnob

PedalKnob::PedalKnob (const Image& knobArt) : art (knobArt)
{
    setSliderStyle (Slider::RotaryVerticalDrag);
    setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    setMouseCursor (MouseCursor::UpDownResizeCursor);
    setScrollWheelEnabled (true);
}

float PedalKnob::getKnobRadius() const noexcept
{
    return (float) jmin (getWidth(), getHeight()) / (2.0f * boundsFactor);
}

bool PedalKnob::hitTest (int x, int y)
{
    return insideCircle (*this, x, y, 1.0f / boundsFactor * 1.05f);
}

void PedalKnob::paint (Graphics& g)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float r = getKnobRadius();
    const auto disc = Rectangle<float> (2.0f * r, 2.0f * r).withCentre (c);

    {   // contact shadow on the pedal
        Path p;
        p.addEllipse (disc);
        DropShadow (Colours::black.withAlpha (0.65f), roundToInt (r * 0.22f),
                    { roundToInt (r * 0.03f), roundToInt (r * 0.08f) }).drawForPath (g, p);
    }

    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImage (art, disc, RectanglePlacement::stretchToFit);

    // pointer
    const auto rp = getRotaryParameters();
    const float prop  = (float) valueToProportionOfLength (getValue());
    const float angle = rp.startAngleRadians + prop * (rp.endAngleRadians - rp.startAngleRadians);
    const Point<float> dir (std::sin (angle), -std::cos (angle));

    g.setColour (Colours::white.withAlpha (0.28f));                          // faint tail on the panel
    strokeRound (g, c + dir * (r * 0.72f), c + dir * (r * 1.22f), r * 0.022f);

    g.setColour (Colours::black.withAlpha (0.45f));                          // pointer shadow
    strokeRound (g, c + dir * (r * 0.10f) + Point<float> (r * 0.02f, r * 0.03f),
                    c + dir * (r * 0.62f) + Point<float> (r * 0.02f, r * 0.03f), r * 0.072f);

    g.setColour (Colour (0xfff6f2e8));                                       // pointer
    strokeRound (g, c + dir * (r * 0.10f), c + dir * (r * 0.62f), r * 0.056f);

    if (hasKeyboardFocus (false))
    {
        g.setColour (Colours::white.withAlpha (0.25f));
        g.drawEllipse (disc.reduced (2.0f), 1.5f);
    }
}

//==============================================================================
// LeverSwitch

LeverSwitch::LeverSwitch()
{
    setClickingTogglesState (true);
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void LeverSwitch::tick()
{
    const float target = getToggleState() ? 0.0f : 1.0f;   // toggle ON = MUTE = left
    if (std::abs (target - position) > 0.002f)
    {
        position += (target - position) * 0.45f;
        if (std::abs (target - position) < 0.01f)
            position = target;
        repaint();
    }
}

bool LeverSwitch::hitTest (int x, int y)
{
    return insideCircle (*this, x, y, 1.0f);
}

void LeverSwitch::paintButton (Graphics& g, bool, bool)
{
    const float k = (float) getWidth() / 110.0f;               // component is 110 artwork-pixels wide
    const auto hole = getLocalBounds().toFloat().getCentre();
    const auto ball = hole + Point<float> ((position - 0.5f) * 30.0f * k, 5.0f * k);
    const float br = 21.0f * k;

    g.setColour (Colour (0xff4a4943));                          // lever shaft
    strokeRound (g, hole + Point<float> (0.0f, 2.0f * k), ball, 15.0f * k);

    g.setColour (Colours::black.withAlpha (0.5f));              // ball shadow
    g.fillEllipse (Rectangle<float> (br * 2.1f, br * 2.1f).withCentre (ball + Point<float> (2.5f * k, 3.5f * k)));

    ColourGradient grad (Colour (0xfff2f2ee), ball.x - br * 0.35f, ball.y - br * 0.45f,
                         Colour (0xff25251f), ball.x + br * 0.85f, ball.y + br * 0.85f, true);
    grad.addColour (0.45, Colour (0xff8d8d86));
    g.setGradientFill (grad);
    g.fillEllipse (Rectangle<float> (br * 2.0f, br * 2.0f).withCentre (ball));

    g.setColour (Colours::white.withAlpha (0.85f));             // specular
    g.fillEllipse (Rectangle<float> (br * 0.5f, br * 0.4f).withCentre (ball + Point<float> (-br * 0.35f, -br * 0.45f)));
}

//==============================================================================
// FootSwitchButton

FootSwitchButton::FootSwitchButton()
{
    setClickingTogglesState (true);
    setMouseCursor (MouseCursor::PointingHandCursor);
}

bool FootSwitchButton::hitTest (int x, int y)
{
    return insideCircle (*this, x, y, 1.0f);
}

void FootSwitchButton::paintButton (Graphics& g, bool isOver, bool isDown)
{
    const auto b = getLocalBounds().toFloat().reduced (getWidth() * 0.18f);
    if (isDown)
    {
        g.setColour (Colours::black.withAlpha (0.28f));
        g.fillEllipse (b);
    }
    else if (isOver)
    {
        g.setColour (Colours::white.withAlpha (0.05f));
        g.fillEllipse (b);
    }
}

//==============================================================================
// Editor

DoctorMigranaTunerEditor::DoctorMigranaTunerEditor (DoctorMigranaTunerProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      baseImage (ImageCache::getFromMemory (BinaryData::pedal_base_jpg, BinaryData::pedal_base_jpgSize)),
      knobImage (ImageCache::getFromMemory (BinaryData::knob_png, BinaryData::knob_pngSize)),
      hzKnob (knobImage),
      centsKnob (knobImage),
      hzAttachment     (p.apvts, dmt::ParamID::refHz,    hzKnob),
      centsAttachment  (p.apvts, dmt::ParamID::centsTol, centsKnob),
      muteAttachment   (p.apvts, dmt::ParamID::mute,     muteSwitch),
      activeAttachment (p.apvts, dmt::ParamID::active,   footSwitch)
{
    refParam = p.apvts.getRawParameterValue (dmt::ParamID::refHz);
    tolParam = p.apvts.getRawParameterValue (dmt::ParamID::centsTol);

    hzKnob.setNumDecimalPlacesToDisplay (1);
    hzKnob.setTextValueSuffix (" Hz");
    hzKnob.setDoubleClickReturnValue (true, 440.0);
    hzKnob.setMouseDragSensitivity (400);
    hzKnob.setPopupDisplayEnabled (true, true, this);
    hzKnob.setTooltip ("Reference pitch of A4 (double-click = 440 Hz)");

    centsKnob.setNumDecimalPlacesToDisplay (0);
    centsKnob.setTextValueSuffix (" cents");
    centsKnob.setDoubleClickReturnValue (true, 1.0);
    centsKnob.setMouseDragSensitivity (250);
    centsKnob.setPopupDisplayEnabled (true, true, this);
    centsKnob.setTooltip ("\"In tune\" tolerance, 1 to 25 cents (double-click = 1)");

    muteSwitch.setTooltip ("Left = MUTE the output while the tuner is active.  Right = BYPASS (signal keeps flowing)");
    footSwitch.setTooltip ("Tuner on / off");

    addAndMakeVisible (hzKnob);
    addAndMakeVisible (centsKnob);
    addAndMakeVisible (muteSwitch);
    addAndMakeVisible (footSwitch);

    setResizable (true, true);
    setResizeLimits (293, 424, 835, 1210);
    getConstrainer()->setFixedAspectRatio ((double) L::artW / (double) L::artH);

    const int w = jlimit (293, 835, (int) p.apvts.state.getProperty ("editorWidth", 418));
    setSize (w, roundToInt ((float) w * L::artH / L::artW));

    startTimerHz (30);
}

DoctorMigranaTunerEditor::~DoctorMigranaTunerEditor()
{
    stopTimer();
}

void DoctorMigranaTunerEditor::resized()
{
    const float s = getScale();
    auto place = [s] (Component& c, Point<float> centre, float halfSize)
    {
        c.setBounds (Rectangle<float> (2.0f * halfSize * s, 2.0f * halfSize * s)
                         .withCentre (centre * s).toNearestInt());
    };

    place (hzKnob,    L::hzKnobCentre,    L::knobRadius * PedalKnob::boundsFactor);
    place (centsKnob, L::centsKnobCentre, L::knobRadius * PedalKnob::boundsFactor);
    place (muteSwitch, L::toggleCentre,   55.0f);
    place (footSwitch, L::footSwitchCentre, L::footSwitchRadius);

    proc.apvts.state.setProperty ("editorWidth", getWidth(), nullptr);
}

void DoctorMigranaTunerEditor::paint (Graphics& g)
{
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImage (baseImage, getLocalBounds().toFloat());

    g.addTransform (AffineTransform::scale (getScale()));

    for (int i = 0; i < L::numLeds; ++i)
        drawLitLed (g, { L::ledX[i], L::ledY }, L::ledRadius, ledColour (i), ledLevel[i]);

    drawLitLed (g, L::activeLedCentre, L::activeLedRadius, Colour (0xffff2418), activeLedLevel);
}

void DoctorMigranaTunerEditor::timerCallback()
{
    muteSwitch.tick();

    const bool active = footSwitch.getToggleState();
    const auto reading = proc.getAnalyser().getReading();

    float target[L::numLeds] {};
    int index = -1;

    if (active && reading.valid && reading.frequency > 10.0f)
    {
        const float ref = refParam->load();
        float tol = tolParam->load();

        const double midi    = 69.0 + 12.0 * std::log2 ((double) reading.frequency / (double) ref);
        const double cents   = (midi - std::round (midi)) * 100.0;      // -50 .. +50 vs nearest note
        const double absC    = std::abs (cents);

        if (lastLedIndex == 4)                  // hysteresis so the centre LED doesn't flicker at the edge
            tol += 0.25f;

        if (absC <= (double) tol)
        {
            index = 4;
        }
        else
        {
            const double span  = std::max (50.0 - (double) tol, 1.0);
            const int    steps = jlimit (1, 4, (int) std::ceil ((absC - (double) tol) / span * 4.0));
            index = cents < 0.0 ? 4 - steps : 4 + steps;
        }
        target[index] = 1.0f;
    }
    lastLedIndex = index;

    bool changed = false;
    for (int i = 0; i < L::numLeds; ++i)
    {
        const float old = ledLevel[i];
        ledLevel[i] = target[i] > old ? old + (target[i] - old) * 0.7f
                                      : old * 0.80f;
        if (ledLevel[i] < 0.01f) ledLevel[i] = 0.0f;
        changed |= std::abs (ledLevel[i] - old) > 0.001f;
    }

    const float oldActive = activeLedLevel;
    const float activeTarget = active ? 1.0f : 0.0f;
    activeLedLevel += (activeTarget - activeLedLevel) * 0.6f;
    if (std::abs (activeLedLevel - activeTarget) < 0.01f) activeLedLevel = activeTarget;
    const bool activeChanged = std::abs (activeLedLevel - oldActive) > 0.001f;

    const float s = getScale();
    if (changed)
        repaint (Rectangle<float> (40.0f, 530.0f, 750.0f, 150.0f).transformedBy (AffineTransform::scale (s)).getSmallestIntegerContainer());
    if (activeChanged)
        repaint (Rectangle<float> (90.0f, 840.0f, 160.0f, 170.0f).transformedBy (AffineTransform::scale (s)).getSmallestIntegerContainer());
}
