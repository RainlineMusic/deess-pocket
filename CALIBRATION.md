# Vocal calibration

Offline-rendered `lera_vox_01.wav` (48 kHz, mono, 45.30 s) using Threshold
−52.2 dBFS, Ratio 4:1, Low 0 dB, Sibilance Gain 0 dB. The 2048-point frame
centers need not coincide exactly with handwritten timestamp boundaries.

| Marked time | Peak confidence within ±0.1 s | Detected frames | Peak repair |
| ---: | ---: | ---: | ---: |
| 6.2 ч | .97 | 9 | 5.0 dB |
| 6.5 с | 1.00 | 24 | 3.6 dB |
| 6.8 ц | .98 | 12 | 6.2 dB |
| 7.3 с | .92 | 7 | 7.2 dB |
| 10.0 с | .99 | 12 | 9.8 dB |
| 14.8 с | 1.00 | 9 | 9.2 dB |
| 15.4 с | .99 | 6 | 8.1 dB |
| 16.1 с | 1.00 | 20 | 7.4 dB |
| 16.7 ц | 1.00 | 24 | 11.7 dB |

All nine marked events are found. This is only a recall check; the unlabelled
events and false positives require listening. The supplied short dry/WET pair
processed by SpecCraft demonstrates the intended result, but equal numerical
threshold values do not guarantee identical spectral cuts: FFT normalization,
internal slope, and filtering are not published. Tune the plugin by ear against
that pair, especially for whistles and high-frequency grain.
