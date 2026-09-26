# Deess Pocket — current DSP and interface specification

## Audio

The linked stereo detector identifies sibilant frames before any user threshold
is applied. It uses high-frequency energy, spectral share, centroid, flatness,
level, and hysteresis. The 2048-sample STFT with 256-sample hops provides
2047 samples of fixed latency, including bypass. A common mask on both channels
preserves stereo balance.

Four controls, all automatable:

| Parameter | Range / default | Processing |
| --- | --- | --- |
| Threshold | −90…0 dBFS / −52.2 dBFS | Horizontal absolute per-bin threshold in the spectral band above 3 kHz when a sibilant is detected. |
| Ratio | 1:1…20:1 and ∞:1 / 4:1 | Reduces each bin's positive excess over Threshold by `1 − 1/ratio`; at ∞:1 the full excess is removed. No range cap. |
| Low | −12…0 dB / 0 dB | Dynamic low shelf with 600 Hz midpoint, fading between about 300 Hz and 1.2 kHz. It protects tonal voiced lows during overlapping consonants. |
| Sibilance Gain | −12…+12 dB / 0 dB | Adds the same signed dB gain to every detected sibilant, regardless of its original level. |

Repair uses approximately 1 ms attack and 30 ms release for each spectral bin,
starting only at 3 kHz with a short 3–3.5 kHz transition. The detector's
high-band sidechain starts at 3 kHz. Low has its own fast 6 ms release and
turns down when the sub-600 Hz spectrum is tonal, protecting overlapping voiced
material. The event gain envelope uses 1/30 ms timing. Low and Sibilance Gain
do not alter vowels while the gate is closed. The three dB contributions sum into one
spectral gain mask and one inverse STFT. The processing threshold is measured
on the plugin's normalized Hann-window FFT bins; the numeric −52.2 dB setting
is a starting value, not a calibration claim about SpecCraft's display.

The detector is a heuristic. It identifies all nine user-marked consonants in
the supplied vocal, but further listening and false-positive evaluation are
needed for breaths, bleed, plosives and mixed music.

## Visual design for JUCE

Canvas: 1536 × 922 reference coordinates, uniformly scaled at 100%, 110%,
125%, 150% or 200%. Use SF Pro Display on macOS, Segoe UI on Windows.

| Element | Specification |
| --- | --- |
| Header | y=0–103; centered thin DEESS near y=11–58; tiny tracked POCKET SERIES near y=55–75; three menu hairlines x=37–64; power ring centered around 1486,47. |
| Main field | Deep navy layered radial and vertical gradient, slightly brighter around spectrum and controls, dark edges. No visible rectangular button backgrounds. |
| Analyzer | Live 1024-point logarithmic plot, frequency 20 Hz–20 kHz, x=36–1480; bright pearl contour, several translucent glow strokes and a white/steel blue fill fading into the dark lower field. |
| Grid | Hairline logarithmic verticals, subtle 6 dB horizontals, labels at y≈885 and right edge. Keep it under the live signal. |
| Repair | Thin magenta horizontal input threshold indicator starting at 3 kHz; one vivid magenta response curve with a soft multi-pass glow, tracking the signed combined gain. |
| Controls | Centers x=475,666,855,1048; labels near y=690; dark shaded 112 px face, luminous fine arcs and short white pointer; values near y=844. Labels THRESHOLD, RATIO, LOW, SIBILANCE GAIN. |
| Bypass | Header and controls in header remain sharp. Blur/dim only y≥104, with centered BYPASS text. Cache the blurred lower snapshot before a click; visual state flips immediately. |

The native JUCE drawing contains live graph paths and vector controls. The
embedded background and dial-face images are generated assets, not a flattened
image of the reference. Glow is drawn into quarter-resolution images, blurred
and cached: the response about 15 times a second and each dial only when its
value changes. This keeps the diffuse light independent of the audio thread
without a Skia dependency. Bypass snapshot blur also happens only on the
message thread at low resolution, never on the audio callback or click.

Spectrum menu: Fast/Medium/Slow, FFT resolution 1024/2048/4096/8192, display
range 60/90/120 dB, tilt 0/3/4.5 dB per octave. Defaults Fast/High/90/+3.
These settings change only the display.

## Validation

Measure input/processed/delta at each marked consonant and listen for lisp,
chirp, pumping, breaths and plosives. Verify automation, state restore, block
sizes, sample rates, stereo balance, impulse latency, host bypass and UI
performance in actual DAWs. CI builds macOS universal and Windows x64 VST3/AAX
and runs core tests and pluginval; unsigned CI binaries need signing for release.
