// SPDX-License-Identifier: AGPL-3.0-or-later
// Standalone accuracy test: no JUCE needed.  g++ -O2 -std=c++17 -I Source Tests/test_pitch_detector.cpp
#include "PitchDetector.h"
#include <cstdio>
#include <random>

#ifndef M_PI   // MSVC does not define it by default
#define M_PI 3.14159265358979323846
#endif

int main()
{
    PitchDetector pd;
    const double fs = 48000.0;
    pd.prepare (fs);
    const int N = pd.getRequiredSamples();
    std::vector<float> x ((size_t) N);
    std::mt19937 g (1);
    std::normal_distribution<float> noise (0.0f, 0.003f);

    double worst = 0.0;
    const double notes[] = { 41.2034, 82.4069, 110.0, 146.83, 196.0, 246.94, 329.63, 440.0, 659.26, 880.0, 1318.5 };
    for (double f0 : notes)
        for (int off = -50; off <= 50; off += 5)
        {
            const double f = f0 * std::pow (2.0, off / 1200.0);
            for (int i = 0; i < N; ++i)
            {
                const double t = i / fs;
                x[(size_t) i] = 0.4f * (float) (std::sin (2 * M_PI * f * t) + 0.5 * std::sin (4 * M_PI * f * t + 0.3)
                                              + 0.25 * std::sin (6 * M_PI * f * t + 1.0) + 0.1 * std::sin (8 * M_PI * f * t))
                              + noise (g);
            }
            auto r = pd.process (x.data(), N);
            const double c = r.valid ? 1200.0 * std::log2 (r.frequency / f) : 999.0;
            worst = std::max (worst, std::abs (c));
            if (std::abs (c) > 1.0)
                std::printf ("FAIL f0=%.1f off=%d  err=%.3f cents valid=%d\n", f0, off, c, (int) r.valid);
        }
    std::printf ("worst error: %.3f cents\n", worst);
    return worst < 1.0 ? 0 : 1;
}
