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
    engine.setControls({-52.2f, 1.0f, 0.0f, 0.0f, false});
    std::vector<float> in(count), out(count);
    for (int i = 0; i < count; ++i)
        in[i] = 0.3f * std::sin(2.0 * 3.141592653589793 * 330.0 * i / 48000.0);
    for (int i = 0; i < count; ++i) engine.process(in[i], in[i], out[i], out[i]);
    float error = 0.0f;
    for (int i = 4096; i < count; ++i)
        error = std::max(error, std::abs(out[i] - in[i - engine.latencySamples()]));
    if (error > 0.001f) { std::cerr << "Transparent path: " << error << '\n'; return 1; }

    std::mt19937 rng(12345);
    std::uniform_real_distribution<float> random(-1.0f, 1.0f);
    engine.reset();
    engine.setControls({-52.2f, 4.0f, -12.0f, 3.0f, false});
    float confidence = 0.0f, repair = 0.0f, eventGain = 0.0f;
    for (int i = 0; i < count; ++i) {
        const float x = i > 12000 && i < 34000 ? 0.3f * random(rng) : 0.0f;
        float l, r; engine.process(x, x, l, r);
        if (!std::isfinite(l)) return 2;
        const auto m = engine.meters();
        confidence = std::max(confidence, m.confidence);
        repair = std::max(repair, m.repairPeakDb);
        eventGain = std::max(eventGain, m.eventGainDb);
    }
    if (confidence < 0.52f || repair < 3.0f || eventGain < 2.8f) {
        std::cerr << "Frication: " << confidence << ", " << repair << ", " << eventGain << '\n';
        return 3;
    }

    // A fricative overlapping a voiced tone must not let Low pull down the
    // 220 Hz voice. Repair itself is restricted to bins at/above 3 kHz.
    auto voicedProjection = [&](float lowDb) {
        engine.reset();
        engine.setControls({-72.0f, 21.0f, lowDb, 0.0f, false});
        std::mt19937 generator(31337);
        double projection = 0.0;
        for (int i = 0; i < count; ++i) {
            const float voiced = 0.2f * std::sin(2.0 * 3.141592653589793 * 220.0 * i / 48000.0);
            const float x = i > 8000 && i < 40000 ? voiced + 0.18f * random(generator) : voiced;
            float l, r; engine.process(x, x, l, r);
            if (i > 17000 && i < 35000) {
                const int aligned = i - engine.latencySamples();
                projection += l * std::sin(2.0 * 3.141592653589793 * 220.0 * aligned / 48000.0);
            }
        }
        return projection;
    };
    const double normalVoice = voicedProjection(0.0f);
    const double protectedVoice = voicedProjection(-12.0f);
    if (normalVoice < 100.0 || protectedVoice / normalVoice < 0.90) {
        std::cerr << "Low touched voiced overlap: " << protectedVoice / normalVoice << '\n';
        return 8;
    }

    engine.reset();
    engine.setControls({-90.0f, 21.0f, -12.0f, 12.0f, false});
    float falseAction = 0.0f;
    for (int i = 0; i < count; ++i) {
        const float vowel = 0.28f * std::sin(2.0 * 3.141592653589793 * 200.0 * i / 48000.0)
                          + 0.12f * std::sin(2.0 * 3.141592653589793 * 400.0 * i / 48000.0);
        float l, r; engine.process(vowel, vowel, l, r);
        const auto m = engine.meters();
        falseAction = std::max({falseAction, m.repairPeakDb, m.eventGainDb});
    }
    if (falseAction > 0.1f) { std::cerr << "Vowel altered: " << falseAction << '\n'; return 4; }

    engine.reset();
    engine.setControls({-90.0f, 21.0f, -12.0f, 12.0f, true});
    error = 0.0f;
    for (int i = 0; i < count; ++i) {
        float l, r; engine.process(in[i], in[i], l, r);
        if (i > 4096) error = std::max(error, std::abs(l - in[i - engine.latencySamples()]));
    }
    if (error > 1.0e-6f) { std::cerr << "Bypass latency: " << error << '\n'; return 5; }
    for (double rate : {44100.0, 96000.0}) {
        engine.prepare(rate, 1);
        engine.setControls({-52.2f, 4.0f, 0.0f, 0.0f, false});
        std::mt19937 generator(static_cast<unsigned>(rate));
        float maximum = 0.0f;
        const int samples = static_cast<int>(rate * 0.6);
        for (int i = 0; i < samples; ++i) {
            const float x = i > samples / 4 && i < 3 * samples / 4
                ? 0.25f * random(generator) : 0.0f;
            float l, r; engine.process(x, x, l, r);
            if (!std::isfinite(l)) return 6;
            maximum = std::max(maximum, engine.meters().confidence);
        }
        if (maximum < 0.52f) return 7;
    }
    std::cout << "PASS reconstruction, spectral repair, event gain, vowel gate, bypass\n";
}
