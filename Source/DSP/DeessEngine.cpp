#include "DeessEngine.h"
#include <algorithm>
#include <cmath>

namespace deess {
namespace {
constexpr float pi = 3.14159265358979323846f;
float clamp01(float x) noexcept { return std::clamp(x, 0.0f, 1.0f); }
float smooth(float a, float b, float x) noexcept {
    const float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}
}

float Engine::db(float a) noexcept { return 20.0f * std::log10(std::max(a, 1.0e-9f)); }
float Engine::linear(float d) noexcept { return std::pow(10.0f, d / 20.0f); }

void Engine::prepare(double sampleRate, int channelCount) {
    rate = std::max(8000.0, sampleRate);
    channels = std::clamp(channelCount, 1, 2);
    for (int n = 0; n < fftSize; ++n)
        window[n] = std::sqrt(0.5f - 0.5f * std::cos(2.0f * pi * n / fftSize));
    const float binHz = static_cast<float>(rate / fftSize);
    for (int k = 0; k <= fftSize / 2; ++k) {
        const float hz = k * binHz;
        lowShelfWeight[k] = 1.0f - smooth(std::log2(300.0f), std::log2(1200.0f),
                                          std::log2(std::max(20.0f, hz)));
        // Repair has no action below 3 kHz; soften the upper edge only.
        repairBandWeight[k] = smooth(3000.0f, 3500.0f, hz);
    }
    bypassCoefficient = std::exp(-1.0f / static_cast<float>(rate * 0.002));
    repairAttack = std::exp(-float(hopSize / rate) / 0.001f);
    repairRelease = std::exp(-float(hopSize / rate) / 0.030f);
    lowRelease = std::exp(-float(hopSize / rate) / 0.006f);
    reset();
}

void Engine::reset() noexcept {
    clock = 0;
    confidenceSmoothed = 0.0f;
    noiseDb = -85.0f;
    eventEnvelope = 0.0f;
    lowEnvelope = 0.0f;
    bypassMix = controls.bypass ? 1.0f : 0.0f;
    eventActive = false;
    meter = {};
    meter.detectorDb = -120.0f;
    for (auto& x : input) x.fill(0.0f);
    for (auto& x : dry) x.fill(0.0f);
    for (auto& x : overlap) x.fill(0.0f);
    norm.fill(0.0f);
    repairSmoothed.fill(0.0f);
}

void Engine::setControls(Controls c) noexcept {
    controls.thresholdDb = std::clamp(std::isfinite(c.thresholdDb) ? c.thresholdDb : -52.2f,
                                      -90.0f, 0.0f);
    controls.ratio = std::clamp(std::isfinite(c.ratio) ? c.ratio : 4.0f, 1.0f, 21.0f);
    controls.lowDb = std::clamp(std::isfinite(c.lowDb) ? c.lowDb : 0.0f, -12.0f, 0.0f);
    controls.sibilanceGainDb = std::clamp(std::isfinite(c.sibilanceGainDb) ? c.sibilanceGainDb : 0.0f,
                                          -12.0f, 12.0f);
    controls.bypass = c.bypass;
}

void Engine::fft(Spectrum& a, bool inverse) noexcept {
    for (int i = 1, j = 0; i < fftSize; ++i) {
        int bit = fftSize >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (int len = 2; len <= fftSize; len <<= 1) {
        const float phase = (inverse ? 2.0f : -2.0f) * pi / len;
        const std::complex<float> step(std::cos(phase), std::sin(phase));
        for (int base = 0; base < fftSize; base += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (int k = 0; k < len / 2; ++k) {
                const auto u = a[base + k], v = a[base + k + len / 2] * w;
                a[base + k] = u + v;
                a[base + k + len / 2] = u - v;
                w *= step;
            }
        }
    }
    if (inverse)
        for (auto& x : a) x /= static_cast<float>(fftSize);
}

void Engine::processFrame(std::int64_t start) noexcept {
    constexpr int bins = fftSize / 2 + 1;
    const float binHz = static_cast<float>(rate / fftSize);
    for (int ch = 0; ch < channels; ++ch) {
        for (int n = 0; n < fftSize; ++n) {
            const auto t = start + n;
            spectra[ch][n] = {(t >= 0 ? input[ch][t % fftSize] : 0.0f) * window[n], 0.0f};
        }
        fft(spectra[ch], false);
    }
    float low = 1.0e-12f, high = 1.0e-12f, mid = 1.0e-12f, total = 1.0e-12f;
    float centroid = 0.0f, logSum = 0.0f, arithmetic = 0.0f;
    float lowLogSum = 0.0f, lowArithmetic = 0.0f;
    int highBins = 0, lowBins = 0;
    for (int k = 0; k < bins; ++k) {
        float magnitude = std::abs(spectra[0][k]);
        if (channels == 2) magnitude = std::max(magnitude, std::abs(spectra[1][k]));
        magnitudes[k] = db(magnitude / (fftSize * 0.5f));
        const float hz = k * binHz;
        const float energy = magnitude * magnitude;
        if (hz >= 150.0f && hz < 1500.0f) low += energy;
        if (hz >= 1500.0f && hz < 3000.0f) mid += energy;
        if (hz >= 3000.0f && hz <= std::min(14000.0f, static_cast<float>(rate * 0.48))) {
            high += energy;
            arithmetic += energy;
            logSum += std::log(energy + 1.0e-12f);
            ++highBins;
        }
        if (hz >= 80.0f && hz < 600.0f) {
            lowArithmetic += energy;
            lowLogSum += std::log(energy + 1.0e-12f);
            ++lowBins;
        }
        if (hz >= 150.0f && hz < 14000.0f) {
            total += energy;
            centroid += hz * energy;
        }
    }
    const float hfDb = db(std::sqrt(high) / (fftSize * 0.5f));
    const float hfShare = high / (low + mid + high);
    const float spectralCentroid = centroid / total;
    const float flatness = highBins > 0
        ? std::exp(logSum / highBins) / (arithmetic / highBins + 1.0e-12f) : 0.0f;
    const float lowFlatness = lowBins > 0
        ? std::exp(lowLogSum / lowBins) / (lowArithmetic / lowBins + 1.0e-12f) : 0.0f;
    // Detector confidence is independent of the user's threshold. A low threshold
    // can never make a vowel qualify as a fricative by itself.
    const float presence = smooth(3.0f, 18.0f, hfDb - noiseDb)
                           * smooth(-78.0f, -60.0f, hfDb);
    const float score = (0.45f * smooth(0.18f, 0.70f, hfShare)
                       + 0.35f * smooth(2600.0f, 6500.0f, spectralCentroid)
                       + 0.20f * smooth(0.06f, 0.35f, flatness)) * presence;
    const float frameSeconds = static_cast<float>(hopSize / rate);
    const float probabilityTime = score > confidenceSmoothed ? 0.006f : 0.025f;
    const float p = std::exp(-frameSeconds / probabilityTime);
    confidenceSmoothed = score + p * (confidenceSmoothed - score);
    if (!eventActive && confidenceSmoothed >= 0.52f) eventActive = true;
    else if (eventActive && confidenceSmoothed <= 0.27f) eventActive = false;
    // Update a quiet reference only outside likely fricative events.
    if (!eventActive) noiseDb = std::min(hfDb, noiseDb + 0.002f);

    const float eventTarget = eventActive ? 1.0f : 0.0f;
    const float eventCoeff = eventActive ? repairAttack : repairRelease;
    eventEnvelope = eventTarget + eventCoeff * (eventEnvelope - eventTarget);
    // A simultaneous voiced vowel carries coherent harmonics below 600 Hz.
    // Protect that tonal low end even while the high-band fricative gate is open.
    const float lowTarget = eventActive ? smooth(0.10f, 0.38f, lowFlatness) : 0.0f;
    lowEnvelope = lowTarget + (lowTarget > lowEnvelope ? repairAttack : lowRelease)
                            * (lowEnvelope - lowTarget);
    meter.confidence = confidenceSmoothed;
    meter.detectorDb = hfDb;
    meter.event = eventActive;
    meter.eventGainDb = eventEnvelope * controls.sibilanceGainDb;
    meter.repairPeakDb = 0.0f;

    // One absolute threshold for every FFT bin; no adaptive spectral envelope.
    for (int k = 0; k < bins; ++k) {
        const float slope = controls.ratio >= 20.5f ? 1.0f : 1.0f - 1.0f / controls.ratio;
        const float targetRepair = eventActive
            ? std::max(0.0f, magnitudes[k] - controls.thresholdDb) * slope
              * repairBandWeight[k] : 0.0f;
        const float rCoeff = targetRepair > repairSmoothed[k] ? repairAttack : repairRelease;
        repairSmoothed[k] = targetRepair + rCoeff * (repairSmoothed[k] - targetRepair);
        meter.repairPeakDb = std::max(meter.repairPeakDb, repairSmoothed[k]);
        const float netDb = eventEnvelope * controls.sibilanceGainDb
                            + lowEnvelope * controls.lowDb * lowShelfWeight[k]
                            - repairSmoothed[k];
        const float gain = linear(netDb);
        for (int ch = 0; ch < channels; ++ch) {
            spectra[ch][k] *= gain;
            if (k != 0 && k != fftSize / 2) spectra[ch][fftSize - k] *= gain;
        }
    }
    for (int i = 0; i < displayBands; ++i) {
        const float f = 20.0f * std::pow(1000.0f, i / float(displayBands - 1));
        const int k = std::clamp(static_cast<int>(std::round(f / binHz)), 0, bins - 1);
        meter.preSpectrumDb[i] = magnitudes[k];
        meter.gainDb[i] = eventEnvelope * controls.sibilanceGainDb
                          + lowEnvelope * controls.lowDb * lowShelfWeight[k]
                          - repairSmoothed[k];
    }
    for (int ch = 0; ch < channels; ++ch) {
        fft(spectra[ch], true);
        for (int n = 0; n < fftSize; ++n) {
            const auto t = start + n;
            if (t < 0) continue;
            const int slot = static_cast<int>(t % ringSize);
            overlap[ch][slot] += spectra[ch][n].real() * window[n];
            if (ch == 0) norm[slot] += window[n] * window[n];
        }
    }
}

void Engine::process(float left, float right, float& outLeft, float& outRight) noexcept {
    left = std::isfinite(left) ? left : 0.0f;
    right = std::isfinite(right) ? right : 0.0f;
    if (channels == 1) right = left;
    const int inSlot = static_cast<int>(clock % fftSize);
    const int ringSlot = static_cast<int>(clock % ringSize);
    input[0][inSlot] = left; input[1][inSlot] = right;
    dry[0][ringSlot] = left; dry[1][ringSlot] = right;
    if ((clock + 1) % hopSize == 0) processFrame(clock - fftSize + 1);
    outLeft = outRight = 0.0f;
    if (clock >= latencySamples()) {
        const int slot = static_cast<int>((clock - latencySamples()) % ringSize);
        const float reciprocal = norm[slot] > 1.0e-8f ? 1.0f / norm[slot] : 0.0f;
        const float goal = controls.bypass ? 1.0f : 0.0f;
        bypassMix = goal + bypassCoefficient * (bypassMix - goal);
        outLeft = (1.0f - bypassMix) * overlap[0][slot] * reciprocal + bypassMix * dry[0][slot];
        outRight = (1.0f - bypassMix) * overlap[channels - 1][slot] * reciprocal
                 + bypassMix * dry[channels - 1][slot];
        overlap[0][slot] = overlap[1][slot] = norm[slot] = 0.0f;
    }
    ++clock;
}
} // namespace deess
