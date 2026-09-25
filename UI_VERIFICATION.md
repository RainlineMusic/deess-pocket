# UI verification against the supplied 1536 × 922 reference

The JUCE layout uses the reference canvas directly and scales it uniformly for
100%, 110%, 125%, 150% and 200%. The checked preview is
`Design/ui-preview.png`.

| Property | Reference | Implementation |
|---|---:|---:|
| Canvas | 1536 × 922 | 1536 × 922 base coordinates |
| Header lower edge | y≈103 | y=103 |
| Menu icon | x≈37–64 | x=37–64; larger invisible hit target |
| Power ring | centre≈1486,47; diameter≈41 | centre=1485.5,46.5; diameter=41 |
| Dial centres | ≈475, 666, 855, 1048 | 475, 666, 855, 1048 |
| Dial face | ≈112 px | 112 px at 100% |
| Dial labels | y≈690 | centre y=690 |
| Values | y≈844 | centre y=844 |
| Frequency baseline | y≈885 | centre y=885 |

The background is generated artwork with the dark navy base and several broad
blue/cyan glows. It is embedded as a JUCE BinaryData resource. Dial faces are
also rendered from an embedded high-resolution asset. Active arcs use a wide
transparent stroke below the sharp colored stroke, reproducing the glow rather
than drawing only a solid line.

macOS requests SF Pro Display; Windows requests Segoe UI. The title uses extra
tracking and the subtitle uses a smaller, wider tracking value. JUCE falls back
to the platform sans face only when the requested family is unavailable.

The analyzer is a real pre-processing spectrum. Its resolution menu selects
1024, 2048, 4096 or 8192 FFT points; Fast/Medium/Slow changes release, Range
changes vertical scale, and Tilt rotates the displayed spectrum about 1 kHz.
These preferences do not change audio processing.

Bypass captures and Gaussian-blurs only the region below y=104. The title bar,
menu and power icon remain sharp. The centre overlay reads `BYPASS`. Host bypass
and the plugin's power button both activate this view; audio remains latency
aligned.

One intentional difference from the still reference: reduction curves show the
current DSP state. Wide is drawn as full-band reduction, Split as the 4 kHz
high shelf, and Repair as the live spectral mask. They lie at 0 dB when the
processor is inactive instead of displaying decorative fixed cuts.

The preview is a deterministic visual QA render made from the same reference
coordinates and generated assets. The JUCE editor still needs a real macOS and
Windows build check because this container does not contain the JUCE SDK or a
windowing build toolchain.
