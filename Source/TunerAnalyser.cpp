// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner  |  Copyright (C) 2026 GuitarFros
#include "TunerAnalyser.h"
#include <algorithm>
#include <cmath>
#include <vector>

TunerAnalyser::TunerAnalyser() : juce::Thread ("DoctorMigrana pitch analysis") {}

TunerAnalyser::~TunerAnalyser() { stopThread (2000); }

void TunerAnalyser::prepare (double sampleRate)
{
    stopThread (2000);

    // Keep the analysis rate close to 48 kHz so that 1 cent resolution is
    // available even at the top of the guitar range.
    decimation = juce::jmax (1, (int) std::lround (sampleRate / 48000.0));
    const double effectiveRate = sampleRate / (double) decimation;

    detector.prepare (effectiveRate, 2048, 28.0f, 1600.0f);
    needed = detector.getRequiredSamples();

    ring = std::make_unique<std::atomic<float>[]> ((size_t) ringSize);
    for (int i = 0; i < ringSize; ++i)
        ring[(size_t) i].store (0.0f, std::memory_order_relaxed);

    writePos.store (0);
    accum = 0.0f;  accCount = 0;  localPos = 0;
    hasValue.store (false);

    startThread();
}

void TunerAnalyser::release()
{
    stopThread (2000);
}

void TunerAnalyser::push (const float* const* channels, int numChannels, int numSamples) noexcept
{
    if (ring == nullptr || numChannels <= 0)
        return;

    const float invCh  = 1.0f / (float) numChannels;
    const float invDec = 1.0f / (float) decimation;

    for (int i = 0; i < numSamples; ++i)
    {
        float s = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            s += channels[ch][i];

        accum += s * invCh;
        if (++accCount >= decimation)
        {
            ring[(size_t) (localPos & ringMask)].store (accum * invDec, std::memory_order_relaxed);
            ++localPos;
            accum = 0.0f;
            accCount = 0;
        }
    }
    writePos.store (localPos, std::memory_order_release);
}

TunerAnalyser::Reading TunerAnalyser::getReading() const noexcept
{
    Reading r;
    if (! hasValue.load())
        return r;

    const auto age = juce::Time::getMillisecondCounter() - lastValidMs.load();
    r.valid      = age < holdMs;
    r.frequency  = outFrequency.load();
    r.confidence = outConfidence.load();
    return r;
}

void TunerAnalyser::run()
{
    std::vector<float> block ((size_t) needed);
    std::vector<float> history;                // last few estimates (Hz)
    history.reserve (8);
    int misses = 0;
    bool wasEnabled = false;

    while (! threadShouldExit())
    {
        wait (20);
        if (threadShouldExit())
            break;

        if (! enabled.load (std::memory_order_relaxed))
        {
            if (wasEnabled) { history.clear(); hasValue.store (false); }
            wasEnabled = false;
            continue;
        }
        wasEnabled = true;

        const std::int64_t end = writePos.load (std::memory_order_acquire);
        if (end < needed)
            continue;

        const std::int64_t start = end - needed;
        for (int i = 0; i < needed; ++i)
            block[(size_t) i] = ring[(size_t) ((start + i) & ringMask)].load (std::memory_order_relaxed);

        // the audio thread may have overwritten part of what we copied
        if (writePos.load (std::memory_order_acquire) - start > (std::int64_t) ringSize)
            continue;

        const auto r = detector.process (block.data(), needed, 0.0015f, 0.12f);

        if (r.valid && r.confidence > 0.80f)
        {
            misses = 0;

            // new note (jump > 40 cents from the running median): restart smoothing
            if (! history.empty())
            {
                auto sorted = history;
                std::sort (sorted.begin(), sorted.end());
                const float median = sorted[sorted.size() / 2];
                if (std::abs (1200.0f * std::log2 (r.frequency / median)) > 40.0f)
                    history.clear();
            }

            history.push_back (r.frequency);
            if (history.size() > 5)
                history.erase (history.begin());

            // trimmed mean of the last estimates
            auto sorted = history;
            std::sort (sorted.begin(), sorted.end());
            size_t lo = 0, hi = sorted.size();
            if (sorted.size() >= 3) { lo = 1; hi = sorted.size() - 1; }
            double sum = 0.0;
            for (size_t i = lo; i < hi; ++i) sum += sorted[i];

            outFrequency .store ((float) (sum / (double) (hi - lo)));
            outConfidence.store (r.confidence);
            lastValidMs  .store (juce::Time::getMillisecondCounter());
            hasValue     .store (true);
        }
        else if (++misses > 5)
        {
            history.clear();
        }
    }
}
