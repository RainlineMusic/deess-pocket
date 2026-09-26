# Vocal calibration

Offline-rendered `lera_vox_01.wav` (48 kHz, mono, 45.30 s) using Threshold
−52.2 dBFS, Ratio 4:1, Low 0 dB, Sibilance Gain 0 dB. The 2048-point frame
centers need not coincide exactly with handwritten timestamp boundaries.

| Marked time | Peak confidence within ±0.1 s | Detected frames | Peak repair |
| ---: | ---: | ---: | ---: |
| 6.2 ч | .97 | 9 | 6.2 dB |
| 6.5 с | 1.00 | 24 | 4.5 dB |
| 6.8 ц | .95 | 12 | 6.7 dB |
| 7.3 с | .88 | 7 | 8.1 dB |
| 10.0 с | .96 | 12 | 7.4 dB |
| 14.8 с | .98 | 9 | 9.4 dB |
| 15.4 с | .95 | 6 | 5.0 dB |
| 16.1 с | 1.00 | 20 | 5.3 dB |
| 16.7 ц | 1.00 | 24 | 7.5 dB |

All nine marked events are found. This is only a recall check; the unlabelled
events and false positives require listening. The supplied short dry/WET pair
processed by SpecCraft demonstrates the intended result, but equal numerical
threshold values do not guarantee identical spectral cuts: FFT normalization,
internal slope, and filtering are not published. Tune the plugin by ear against
that pair, especially for whistles and high-frequency grain.

The additional `split.wav` and `no split.wav` examples align with the original
vocal at 9.333 s. Their difference is concentrated around 0.7–0.95 s of the
excerpt. With Threshold −52.2, Ratio 6.5:1 and Low −12 dB, the old full-phrase
render reduced voiced lows during the sibilant tail. Tonality-protected Low
makes the revised 0.85–0.95 s output agree with the manually split version to
better than −88 dB RMS difference in each 50 ms window.
