#pragma once
#include <array>
#include <complex>
#include <cstdint>

namespace deess {
constexpr int fftSize = 2048;
constexpr int hopSize = 256;
constexpr int displayBands = 256;

struct Controls {
    float thresholdDb = -24.0f; // -72 .. 0; -72 approximates -infinity only within a confirmed event
    float wide = 0.55f;        // 0 .. 1, maps to 0 .. 12 dB maximum reduction
    float split = 0.70f;       // 0 .. 1, maps to 0 .. 12 dB maximum reduction
    float repair = 0.40f;      // 0 .. 1, lowers local prominence threshold
    bool bypass = false;
};

struct Meters {
    float confidence = 0.0f;
    float detectorDb = -120.0f;
    float wideDb = 0.0f;
    float splitDb = 0.0f;
    float repairPeakDb = 0.0f;
    bool event = false;
    std::array<float, displayBands> preSpectrumDb{};
    std::array<float, displayBands> reductionDb{};
    std::array<float, displayBands> repairDb{};
};

// Fixed latency, stereo-linked spectral processor; allocate only in prepare().
class Engine {
public:
    void prepare(double sampleRate, int channelCount);
    void reset() noexcept;
    void setControls(Controls c) noexcept;
    void process(float left, float right, float& outLeft, float& outRight) noexcept;
    int latencySamples() const noexcept { return fftSize - 1; }
    Meters meters() const noexcept { return meter; }

private:
    using Spectrum = std::array<std::complex<float>, fftSize>;
    static constexpr int ringSize = fftSize * 2;
    void processFrame(std::int64_t start) noexcept;
    static void fft(Spectrum& a, bool inverse) noexcept;
    static float db(float amplitude) noexcept;
    static float linear(float decibels) noexcept;

    double rate = 48000.0;
    int channels = 2;
    std::int64_t clock = 0;
    Controls controls;
    Meters meter;
    float confidenceSmoothed = 0.0f;
    float noiseDb = -85.0f;
    float wideReduction = 0.0f, splitReduction = 0.0f, bypassMix = 0.0f;
    bool eventActive = false;
    std::array<float, fftSize> window{};
    std::array<std::array<float, fftSize>, 2> input{};
    std::array<std::array<float, ringSize>, 2> dry{};
    std::array<std::array<float, ringSize>, 2> overlap{};
    std::array<float, ringSize> norm{};
    std::array<Spectrum, 2> spectra{};
    std::array<float, fftSize / 2 + 1> magnitudes{}, smoothed{};
    std::array<float, fftSize / 2 + 2> prefix{};
    std::array<float, fftSize / 2 + 1> repairSmoothed{}, tiltDb{}, shelfWeight{};
    std::array<int, fftSize / 2 + 1> envelopeLo{}, envelopeHi{};
    float bypassCoefficient = 0.0f, repairAttack = 0.0f, repairRelease = 0.0f;
    float binLevelCorrection = 0.0f;
};
} // namespace deess
