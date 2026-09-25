# Deess Pocket — DSP and JUCE design specification

## 1. Product intent

Treat harsh Russian and other vocal sibilants in three independent but
simultaneous ways: reduce the whole consonant (Wide), reduce its upper band
(Split), and selectively reduce unusually strong spectral peaks (Repair).
The global Threshold controls Wide and Split amount; Repair has its own local
spectral threshold but uses the same consonant event gate. No knob silently
alters another knob's range.

## 2. Signal and control graph

```text
mono/stereo input ──► fixed STFT latency ──► Wide × Split × Repair ──► output
                       │                       ▲       ▲       ▲
                       └─► consonant detector ─┴───────┴───────┘
                                          │
                                      Threshold ──► Wide, Split only
```

The spectrum is analysed in frames. The detector's confidence must be computed
before applying the adjustable Threshold. Hysteresis opens and closes the
event gate. The threshold may be set to −∞ without turning every vowel into
an event. Spectral score alone cannot guarantee Pro-DS-level selectivity;
evaluate false positives with real vocals and refine the detector.

**Wide and Split:** once the gate is open, high-band level above Threshold
determines the gain-reduction drive. Their control percentages map to separate
maximum reductions of 0–12 dB. Wide gain applies to all bins. Split gain
follows a smooth high shelf from 3–6 kHz. Their dB reductions add, so both
100% settings can mean up to 24 dB above the split frequency; this needs
listening validation and perhaps a soft combined guard in a later iteration.

**Repair:** calculate a narrow-smoothed magnitude spectrum and compare it to
a broad relative envelope of the same frame, tilted by +3 dB/oct for detection.
At 0% the stage is off; increasing the control lowers its own threshold from
+14 toward −8 dB relative to that envelope. Attenuate the positive excess, with attack approximately 1 ms
and release 30 ms, after an event is detected. No user-facing Range limit;
bounded numeric operations and smoothing remain necessary. Do not flatten the
natural broad envelope of /с/, /ш/ or /щ/. The static photo's SpecCraft before
and after charts demonstrate one example; they do not establish an ideal
frequency response to impose on all singers.

### Technical cautions

- A true wide path reduces the **entire** spectrum. Any persistent cyan curve
  that acts as a high shelf, as in the mockup, would be visually misleading.
  Draw measured reduction in playback; use sample curves only in mockups.
- The user's 4 kHz split is treated as the midpoint of a transition, not a
  brick-wall split. Abrupt time-varying bin gains introduce ringing and noise.
- Repair's ratio is about excess above its local spectral threshold, not an
  absolute FFT-bin dBFS ceiling. This makes it less sensitive to recording
  level and follows the selected “relative to shape” behavior.
- Two independently calculated STFT outputs would risk phase and latency
  misalignment. Apply one combined gain mask to one STFT stream and perform
  one inverse transform.
- Use the same mask on L/R for a linked mono or stereo vocal. Separate stereo
  detection can be explored later; dual-mono would change stereo balance.
- Always report fixed processing latency, including in bypass. At 44.1, 48,
  96 kHz it is 2047 samples, so the time duration changes with sample rate.

## 3. Visual reference — coordinate brief for JUCE

Reference canvas: **1536 × 922 px**. Use vector/native drawing in reference
coordinates; scale coordinates and fonts together. No flattened screenshot as
the actual UI. Inputs stay attached to JUCE parameters and host automation.

| Element | Reference location / behavior |
| --- | --- |
| Top bar | y=0–103, deep blue-black, thin separator near y=103. |
| Menu icon | Three 27 px hairlines near x=37–64, y=33/41/49. Click target larger than icon. |
| Product title | `DEESS`, centered at x≈768, y≈10–53, size ≈35 px, light weight and spacious tracking. |
| Series subtitle | `POCKET SERIES`, centered near y=60–70, size ≈10 px with wide tracking; thin side rules. |
| Power | Center x≈1486, y≈47; ~41 px circular hairline and simple broken-circle glyph. No text. |
| Main panel | x=0–1536, y=104–922; near-black navy, subtle blue haze, no prominent frame. |
| Grid | Fine vertical logarithmic lines, frequency labels 20, 50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k along y≈885. Horizontal guides lightly drawn every 6 dB, reduction marks on the right. |
| Spectrum | Pearl-grey jagged top line with translucent area fading down. Frequency log from 20 Hz to 20 kHz. Analyzer display sits behind the controls. |
| Reduction traces | Delicate cyan, yellow and magenta paths, visible near top. Show real Wide, Wide+Split and combined reduction, respectively; no hard-coded graphic behavior. |
| Control row | Centers x≈475, 666, 855, 1048; labels at y≈690; arc and 120 px dark face near y≈766; values at y≈844. Lower-center placement. |
| Dial colors | Threshold nearly white, Wide cyan, Split yellow, Repair magenta; dim inactive arc, thin bright active arc, subtle glow, small white radial pointer. |
| Bypass view | Header, menu and power remain sharp and clickable. Blur/dim only y≥104. Large centered `BYPASS` text around the middle of that region. Audio switches via short crossfade to latency-matched dry. |

Rename the screenshot's `SPECTR` label to `REPAIR`. Slider values: Threshold
in dB, the other three as whole percentages. Use the platform's system font:
SF Pro Display on macOS if available, Segoe UI on Windows, native sans fallback
otherwise. The product title should remain thin; kerning and placement matter
more than forcing a particular font weight.

Left menu: scale 100%, 110%, 125%, 150%, 200%. Spectrum submenu: Pre on/off,
Speed Fast/Medium/Slow, Resolution Low/Medium/High/Maximum, Range 60/90/120 dB and Tilt
0/3/4.5 dB/oct. Defaults: Fast, High, 90 dB, 3 dB/oct. These are analyzer
preferences; they must not alter the processing transfer function. The current
The four analyzer resolutions use 1024/2048/4096/8192-point FFTs. The processing
STFT remains fixed at 2048 points and is unaffected by display preferences.

## 4. Acceptance checks for a production candidate

1. Label and render real Russian /с, сь, ш, щ/ at different levels, plus
   vowels, breaths, /т/, hi-hat bleed and consonants overlapping singing.
   Measure recall and false positives at Threshold minimum and typical values.
2. Compare aligned original, processed and delta at each stage alone and in
   combination. Determine whether Repair removes abnormal whistles without
   making /с/ lisp, chirp or collapse into flat noise.
3. Test mono, linked stereo, all target sample rates, host automation,
   silence/denormals, block size changes, save/restore, transport seeking and
   bypass through the DAW and host bypass.
4. Confirm constant latency with an impulse and exact delayed dry bypass;
   build and run VST3/Standalone in JUCE, then pluginval on release binaries.
5. Profile DSP and UI in Pro Tools/Reaper; the display must not affect audio
   callback timing. Test macOS SF and Windows Segoe on standard and HiDPI.

## 5. Documentation reviewed

- [FabFilter Pro-DS basic controls](https://www.fabfilter.com/help/pro-ds/using/basiccontrols): its Single Vocal mode separates sibilance from non-sibilance before thresholding; at −∞ dB its sibilants receive roughly equal reduction.
- [FabFilter Pro-DS advanced controls](https://www.fabfilter.com/help/pro-ds/using/advancedcontrols): wide/full-band versus split-band, up to 15 ms lookahead, and program-dependent choices.
- [Three-Body Technology SpecCraft](https://www.threebodytech.com/en/products/speccraft): spectral resonance suppression, adaptive threshold, lookahead, spectrum slope and compensation. Its actual internals are proprietary; the downloadable manual timed out in this session.
- [FabFilter Pro-Q 4 analyzer](https://www.fabfilter.com/help/pro-q/using/analyzer): range, resolution, speed and tilt definitions. Its High resolution is 4096 points, Maximum 8192, rather than an arbitrary visual-detail setting.
- [JUCE AudioProcessor](https://docs.juce.com/master/classjuce_1_1AudioProcessor.html): latency reporting through `setLatencySamples`.
