# Deess Pocket — experimental JUCE prototype

This is a new project, independent of the earlier `DeessPocket-v0.3.0` source.
It contains a real-time C++17 DSP core, a JUCE VST3/AAX/Standalone wrapper and a
drawn interface based on the user's 1536 × 922 reference image.

## Build

Core tests (no dependency beyond C++17):

```sh
cmake -S . -B build -DDEESS_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Plug-in (JUCE 8 SDK required):

```sh
cmake -S . -B build-plugin -DDEESS_BUILD_PLUGIN=ON -DJUCE_DIR=/path/to/JUCE
cmake --build build-plugin --config Release
```

The GitHub Actions workflow builds universal macOS (arm64 + x86_64) and Windows
x64 VST3 and AAX plug-ins against JUCE 8.0.4, runs the DSP tests, validates
VST3 with `pluginval`, and uploads zipped plug-ins as workflow artifacts. AAX
is included in the same way as the existing Rainline Music JUCE project; no
separate AAX SDK secret is required by this workflow. The CI artifacts are
unsigned developer builds and are not notarized for end-user distribution.

## Current behavior

- Constant 2047-sample latency, reported to the host. At 48 kHz this is
  about 42.6 ms. The STFT analyses the full 2048-sample window before
  applying gain to samples in that window, giving advance information on
  the start of consonants. Latency-aligned bypass is crossfaded in 2 ms.
- Stereo linked event detection and a common spectral mask preserve left/right
  balance. Mono is supported.
- Threshold controls dynamic gain reduction for Wide and Split, only **after**
  the event detector qualifies the sound. Its bottom value is treated as
  effectively −∞ inside an event. Threshold does not drive Repair.
- Wide is full-band attenuation (0–12 dB range). Split is a smooth high shelf
  transitioning between 3 and 6 kHz (0–12 dB range); both can run together.
- Repair compares smoothed FFT magnitudes to a broad relative spectral
  envelope with a 3 dB/oct detection tilt. Its internal threshold goes from
  +14 to −8 dB relative to that envelope across 0–100%. Excess is attenuated at approximately
  ∞:1, without a user Range cap. Detection still gates Repair. There is no
  fixed target spectrum for all /с/, /ш/ and /щ/.
- The 1024-point log display uses independent 1024/2048/4096/8192-point FFT
  resolutions. It defaults to Fast, High resolution, 90 dB display
  range and +3 dB/oct tilt about 1 kHz. Spectrum display settings affect only
  graphics.
- The visible response curves show actual current attenuation, so they become
  flat when no consonant is detected. They do not retain the example notches
  from the static reference while the plug-in is idle.

## Prototype limits

The detector uses spectral energy distribution, centroid, high-band flatness,
level and hysteresis. It is an initial heuristic, not FabFilter's undisclosed
Single Vocal algorithm and not a trained phoneme classifier. It must be
validated on labelled /с, ш, щ/ and non-sibilant vocal recordings, breaths,
plosives and bleed. An isolated broadband synthetic fricative and low sine
vowel pass the included tests; those tests do not prove field performance.

Repair is a **local prominence limiter**. It does not recreate SpecCraft's
proprietary adaptive threshold, compensation or analog-modelled filters.
Without sufficient frequency smoothing, frame-wise suppression can sound
chirpy; without temporal smoothing it can pump. This prototype uses modest
frequency smoothing and 1 ms / 30 ms spectral dynamics as a starting point,
but no blind listening evaluation has been completed.

The processor sends pre-processing samples to the GUI through a bounded SPSC
FIFO and publishes reduction meters atomically. It allocates nothing inside
`processBlock`. The fixed processing STFT is 2048
points, hop 256. UI snapshot scaling for bypass is intentionally cached and
never executed in the audio thread.

See [SPEC.md](SPEC.md) for DSP decisions, [CALIBRATION.md](CALIBRATION.md) for measurements on the supplied audio, and [UI_VERIFICATION.md](UI_VERIFICATION.md) for the reference comparison.
