// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include "PitchDetector.h"

/** Runs the pitch detection on a background thread so the audio callback only
    has to copy samples into a lock-free ring buffer. */
class TunerAnalyser : private juce::Thread
{
public:
    struct Reading
    {
        bool  valid      = false;   // a pitch was detected recently
        float frequency  = 0.0f;    // Hz (smoothed)
        float confidence = 0.0f;
    };

    TunerAnalyser();
    ~TunerAnalyser() override;

    /** Message / setup thread. Must not be called while audio is running. */
    void prepare (double sampleRate);
    void release();

    /** Any thread. When disabled the analysis thread idles. */
    void setEnabled (bool shouldAnalyse) noexcept   { enabled.store (shouldAnalyse, std::memory_order_relaxed); }

    /** Audio thread. Real-time safe: no allocation, no locks. */
    void push (const float* const* channels, int numChannels, int numSamples) noexcept;

    /** Any thread. */
    Reading getReading() const noexcept;

private:
    void run() override;

    static constexpr int ringSize = 1 << 15;            // 32768 samples at the decimated rate
    static constexpr int ringMask = ringSize - 1;
    static constexpr juce::uint32 holdMs = 250;         // keep last pitch this long after the signal vanishes

    PitchDetector detector;
    std::unique_ptr<std::atomic<float>[]> ring;
    std::atomic<std::int64_t> writePos { 0 };

    // audio-thread-only state
    int    decimation = 1;
    int    accCount   = 0;
    float  accum      = 0.0f;
    std::int64_t localPos = 0;

    int needed = 0;
    std::atomic<bool> enabled { false };

    std::atomic<float>         outFrequency  { 0.0f };
    std::atomic<float>         outConfidence { 0.0f };
    std::atomic<juce::uint32>  lastValidMs   { 0 };
    std::atomic<bool>          hasValue      { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TunerAnalyser)
};
