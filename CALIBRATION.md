# Calibration on the supplied vocal and SpecCraft reference

## Vocal detector

The detector was run on `lera_vox_01.wav` (48 kHz, 45.30 s). It opened at all
nine supplied annotations. Because the detector uses a centred 2048-sample
analysis window, reported frame centres and handwritten timestamps can differ
by several tens of milliseconds.

| Annotation | Peak confidence | Peak Repair at 65% |
|---:|---:|---:|
| 6.2 s, ч | 0.966 | 12.60 dB |
| 6.5 s, с | 0.998 | 13.50 dB |
| 6.8 s, ц | 0.979 | 15.59 dB |
| 7.3 s, с | 0.946 | 13.82 dB |
| 10.0 s, с | 0.999 | 13.28 dB |
| 14.8 s, с | 0.997 | 13.51 dB |
| 15.4 s, с | 0.999 | 12.46 dB |
| 16.1 s, с | 1.000 | 11.90 dB |
| 16.7 s, ц | 1.000 | 17.47 dB |

This confirms recall on the marked examples. It does not prove false-positive
performance: the later unlabelled events must be checked by ear or labelled.

## SpecCraft dry/wet pair

The files are sample-aligned and mono duplicated into two channels. The
SpecCraft example reduces the broad upper consonant band, rather than only
isolated single-bin peaks. Its integrated change is approximately:

| Band | SpecCraft | Deess Pocket Repair 65% |
|---|---:|---:|
| 1.8–4 kHz | 0.00 dB | 0.00 dB |
| 4–8 kHz | −0.50 dB | −0.75 dB |
| 8–12 kHz | −6.63 dB | −5.27 dB |
| 12–20 kHz | −5.80 dB | −4.31 dB |

The updated Repair uses a +3 dB/oct detection tilt, a broad relative envelope,
and a relative floor inside the consonant. It therefore follows the supplied
example much more closely while staying independent of input level. It keeps
1 ms attack and 30 ms release. The percentage lowers Repair's own threshold;
there is no hard range cap.

The original numeric threshold of −52.2 dB cannot be copied as an absolute FFT
value: SpecCraft's display calibration and internal normalization are unknown.
The plugin recreates the observed behavior using a relative threshold. This is
also more stable when the vocal recording level changes.

Recommended first listening positions:

- Repair 35–50% for normal cleanup.
- Repair 55–70% for the deliberately strong SpecCraft-style correction.
- Wide/Split at zero while calibrating Repair; then add them after the repaired
  consonant shape is acceptable.
