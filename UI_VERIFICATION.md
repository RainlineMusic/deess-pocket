# Visual comparison

Reference image 1280 × 768; JUCE design canvas 1536 × 922. These share nearly
the same aspect ratio. The normalized locations are maintained:

| Element | Reference scaled to 1536 × 922 | JUCE |
| --- | ---: | ---: |
| Header lower edge | y≈104 | y=103 |
| Menu strokes | x≈37–64 | x=37–64 |
| Title center | x≈768 | x=768 |
| Power center | x≈1486, y≈47 | x=1485.5, y=46.5 |
| Four dial centers | x≈475, 666, 855, 1048 | same |
| Frequency baseline | y≈885 | y≈885 |

The revised background adds layered navy and blue haze with a darker vignette.
The live analyzer uses a brighter pearl contour with multiple faint glow
strokes and a graduated fill. Dial arcs use three soft strokes beneath the
sharp stroke. A single magenta trace shows live combined spectral gain; a
separate thin horizontal line indicates the absolute Repair threshold.

Both header hit targets use a transparent JUCE look and feel, eliminating
default button borders. Bypass freezes a precomputed blurred lower-region
snapshot, dims it and draws a centered BYPASS label. The click handler only
changes state and repaints; the expensive snapshot is made periodically on the
message thread at one-third resolution.

The pixel artwork is not the whole UI: the graph, labels and controls are
drawn natively. `Design/ui-preview.png` is a deterministic illustration
of the updated coordinates and an example live spectrum, not a captured JUCE
editor. A faithful comparison of actual JUCE font rendering, blur and scaled
sizes still requires screenshots of the compiled macOS and Windows plug-in.
