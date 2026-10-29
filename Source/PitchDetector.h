// SPDX-License-Identifier: AGPL-3.0-or-later
// Doctor Migrana - Tuner
// Copyright (C) 2026 GuitarFros
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version. See the LICENSE file for details.
#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

/** Monophonic pitch detector based on YIN (de Cheveigne & Kawahara, 2002),
    with parabolic interpolation on the difference function.
    Pure C++ (no JUCE dependency) so it can be unit-tested on its own. */
class PitchDetector
{
public:
    struct Result
    {
        bool  valid      = false;
        float frequency  = 0.0f;   // Hz
        float confidence = 0.0f;   // 0..1  (1 = perfectly periodic)
        float rms        = 0.0f;
    };

    /** @param sampleRate   rate of the (already decimated) signal
        @param windowSize   number of samples integrated per lag (W)
        @param minFreq/maxFreq  search range in Hz */
    void prepare (double sampleRate, int windowSize = 2048,
                  float minFreq = 28.0f, float maxFreq = 1600.0f)
    {
        fs   = sampleRate;
        W    = windowSize;
        tauMin = std::max (2, (int) std::floor (fs / maxFreq));
        tauMax = (int) std::ceil  (fs / minFreq) + 2;
        d.assign ((size_t) tauMax + 2, 0.0);
        cmnd.assign ((size_t) tauMax + 2, 0.0);
        work.assign ((size_t) (W + tauMax + 2), 0.0f);
    }

    /** Number of input samples required by process(). */
    int getRequiredSamples() const noexcept { return W + tauMax + 2; }

    Result process (const float* x, int numSamples, float gateRms = 0.002f, float threshold = 0.12f)
    {
        Result r;
        const int N = getRequiredSamples();
        if (numSamples < N) return r;

        x += (numSamples - N);                       // use the most recent N samples

        double mean = 0.0;
        for (int i = 0; i < N; ++i) mean += x[i];
        mean /= N;

        double e = 0.0;
        for (int i = 0; i < N; ++i)
        {
            work[(size_t) i] = (float) (x[i] - mean);
            e += (double) work[(size_t) i] * work[(size_t) i];
        }
        r.rms = (float) std::sqrt (e / N);
        if (r.rms < gateRms) return r;

        // 1) difference function
        const float* w = work.data();
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double sum = 0.0;
            for (int j = 0; j < W; ++j)
            {
                const double diff = (double) w[j] - (double) w[j + tau];
                sum += diff * diff;
            }
            d[(size_t) tau] = sum;
        }

        // 2) cumulative mean normalised difference
        cmnd[0] = 1.0;
        double running = 0.0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            running += d[(size_t) tau];
            cmnd[(size_t) tau] = running > 0.0 ? d[(size_t) tau] * tau / running : 1.0;
        }

        // 3) absolute threshold: first dip below threshold, followed down to its local minimum
        int best = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold)
            {
                while (tau + 1 < tauMax && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0)  // fall back to global minimum, accept only if reasonably periodic
        {
            double m = 1e9;
            for (int tau = tauMin; tau < tauMax; ++tau)
                if (cmnd[(size_t) tau] < m) { m = cmnd[(size_t) tau]; best = tau; }
            if (best < 0 || m > 0.35) return r;
        }

        // 4) parabolic interpolation on d(tau) (less biased than on cmnd)
        double tauF = best;
        if (best > 1 && best < tauMax)
        {
            const double s0 = d[(size_t) best - 1], s1 = d[(size_t) best], s2 = d[(size_t) best + 1];
            const double denom = s0 - 2.0 * s1 + s2;
            if (std::abs (denom) > 1e-30)
                tauF = best + 0.5 * (s0 - s2) / denom;
        }

        r.valid      = true;
        r.frequency  = (float) (fs / tauF);
        r.confidence = (float) std::clamp (1.0 - cmnd[(size_t) best], 0.0, 1.0);
        return r;
    }

private:
    double fs = 48000.0;
    int W = 2048, tauMin = 2, tauMax = 1000;
    std::vector<double> d, cmnd;
    std::vector<float> work;
};
