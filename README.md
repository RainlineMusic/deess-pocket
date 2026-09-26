# Deess Pocket

Experimental JUCE sibilance processor for mono or stereo vocals. It detects
consonant events before applying a fixed horizontal spectral threshold.

| Control | Range | Default |
| --- | --- | --- |
| Threshold | −90…0 dBFS | −52.2 dBFS |
| Ratio | 1:1…20:1, then ∞:1 | 4:1 |
| Low | −12…0 dB below 600 Hz during sibilants | 0 dB |
| Sibilance Gain | −12…+12 dB added to every detected consonant | 0 dB |

The spectral Repair acts above 3 kHz; the high-band detector sidechain also
starts at 3 kHz. Low protects tonal voiced lows during consonant overlap.
The 2048-point, 256-hop spectral processor has fixed 2047-sample lookahead
latency (42.6 ms at 48 kHz). Stereo uses a linked detector and gain mask.
Bypass crossfades to latency-aligned dry audio. Detection is independent of
Threshold so a low setting does not automatically process vowels. Attack and
release are approximately 1/30 ms.

## Build

Core tests:

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

GitHub Actions builds universal macOS and Windows x64 VST3 and AAX, runs DSP
tests and validates VST3 with pluginval. Artifacts are unsigned developer
builds.

The detector is heuristic and the fixed dBFS threshold is normalized to this
processor's FFT, not calibrated to SpecCraft's internal scale. The supplied
vocal confirms event detection at the nine marked timestamps; listening and
broader false-positive testing are still needed.

See [SPEC.md](SPEC.md), [CALIBRATION.md](CALIBRATION.md), and
[UI_VERIFICATION.md](UI_VERIFICATION.md).
