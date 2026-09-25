#include "DeessEngine.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

int main() {
    constexpr int count = 48000;
    deess::Engine engine;
    engine.prepare(48000.0, 1);
    engine.setControls({-72.0f, 0.0f, 0.0f, 0.0f, false});
    std::vector<float> in(count), out(count);
    for (int i = 0; i < count; ++i)
        in[i] = 0.3f * std::sin(2.0 * 3.141592653589793 * 330.0 * i / 48000.0);
    for (int i = 0; i < count; ++i) engine.process(in[i], in[i], out[i], out[i]);
    float maxError = 0.0f;
    for (int i = 4096; i < count; ++i)
        maxError = std::max(maxError, std::abs(out[i] - in[i - engine.latencySamples()]));
    if (maxError > 0.001f) {
        std::cerr << "Transparent path error: " << maxError << '\n'; return 1;
    }

    engine.reset();
    engine.setControls({-72.0f, 1.0f, 1.0f, 1.0f, false});
    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> random(-1.0f, 1.0f);
    float detectorMax = 0.0f, reductionMax = 0.0f;
    for (int i = 0; i < count; ++i) {
        // Broadband frication; gate must activate even with a very low threshold.
        const float x = i > 12000 && i < 34000 ? 0.3f * random(rng) : 0.0f;
        float left, right;
        engine.process(x, x, left, right);
        if (!std::isfinite(left)) { std::cerr << "Non-finite output\n"; return 2; }
        const auto m = engine.meters();
        detectorMax = std::max(detectorMax, m.confidence);
        reductionMax = std::max(reductionMax, m.wideDb + m.splitDb);
    }
    if (detectorMax < 0.52f || reductionMax < 1.0f) {
        std::cerr << "Frication missed: " << detectorMax << ", " << reductionMax << '\n'; return 3;
    }
    engine.reset();
    float falseReduction = 0.0f;
    for (int i = 0; i < count; ++i) {
        const float vowel = 0.28f * std::sin(2.0 * 3.141592653589793 * 200.0 * i / 48000.0)
                          + 0.12f * std::sin(2.0 * 3.141592653589793 * 400.0 * i / 48000.0);
        float l, r;
        engine.process(vowel, vowel, l, r);
        const auto m = engine.meters();
        falseReduction = std::max(falseReduction, m.wideDb + m.splitDb + m.repairPeakDb);
    }
    if (falseReduction > 0.1f) {
        std::cerr << "Low threshold acted on vowel: " << falseReduction << '\n'; return 4;
    }
    engine.reset();
    engine.setControls({-72.0f, 1.0f, 1.0f, 1.0f, true});
    maxError = 0.0f;
    for (int i = 0; i < count; ++i) {
        float l, r;
        engine.process(in[i], in[i], l, r);
        if (i > 4096)
            maxError = std::max(maxError, std::abs(l - in[i - engine.latencySamples()]));
    }
    if (maxError > 1.0e-6f) {
        std::cerr << "Bypass latency mismatch: " << maxError << '\n'; return 5;
    }
    for (double rate : {44100.0, 96000.0}) {
        engine.prepare(rate, 1);
        engine.setControls({-72.0f, 0.0f, 0.0f, 0.7f, false});
        std::mt19937 generator(static_cast<unsigned>(rate));
        float maximumConfidence = 0.0f;
        const int samples = static_cast<int>(rate * 0.6);
        for (int i = 0; i < samples; ++i) {
            const float x = i > samples / 4 && i < 3 * samples / 4
                ? 0.25f * random(generator) : 0.0f;
            float l, r;
            engine.process(x, x, l, r);
            if (!std::isfinite(l)) return 6;
            maximumConfidence = std::max(maximumConfidence, engine.meters().confidence);
        }
        if (maximumConfidence < 0.52f) {
            std::cerr << "Frication missed at " << rate << " Hz\n"; return 7;
        }
    }
    std::cout << "PASS reconstruction, fricative gate, vowel rejection, latency-aligned bypass\n";
}
