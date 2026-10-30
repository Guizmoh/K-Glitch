# K-GLITCH

**An experimental glitch instrument for the M-VAVE FM-1.** K-GLITCH is planned as a self-contained sound source for IDM, breakcore and related live performance: no external line input is required.

> **Status: architecture bootstrap.** This repository does not contain a working firmware image. It has not been built for, installed on, or tested with an FM-1. CPU, RAM, audio timing, controls and package compatibility must be measured before claiming hardware support.

## Concept

Generate sound inside the instrument, capture that sound into a small rolling audio buffer, then perform playable micro-loops, stutters, reverses, freezes, scratches and tape stops. A rhythmic engine can vary patterns by bounded probability and human-controlled intensity. The design aims for immediate live control rather than unattended random noise.

## Planned sound path

```text
Synth voices / factory samples -> mixer -> capture ring -> glitch reader -> destruction FX -> stereo output
                                            ^                    ^
                                      capture tap        live controls + clock
```

The capture buffer records the internal mix (or a selectable internal bus); it does not imply a physical line-in. Planned sources include compact oscillators/noise and bundled, license-cleared one-shots. Destruction effects include bit-depth and sample-rate reduction, ring modulation and waveshaping. Each effect must have a bypass and bounded parameters.

## Project status

- [Architecture and technical constraints](docs/ARCHITECTURE.md)
- [Feature roadmap](docs/ROADMAP.md)
- `src/` contains clean-room interface scaffolding only; no KAWA source has been copied.
- Firmware packaging, graphical interface, synthesis, sample playback, capture, DSP and hardware tests remain planned.

## Reference and licensing

The separate project uses [KAWA](https://github.com/Guizmoh/Kawa-Tracker-Fm1) as a technical reference for FM-1 platform facts and build constraints. KAWA identifies its firmware under GPL-3.0-only and credits platform components to Hügelton Instruments/Felucca and others. This bootstrap contains no copied KAWA code, assets, samples, generated tables or build scripts. If compatible code is later adapted, preserve its notices and attribution, comply with GPL-3.0-only and document each reused file and its origin in `THIRD_PARTY_NOTICES.md` before distribution.

This repository's original scaffolding is offered under GPL-3.0-only; see [LICENSE](LICENSE). No affiliation with or endorsement by M-VAVE or Hügelton Instruments is implied.

## Development

There is no build system yet. The next engineering step is to select a minimal FM-1 platform base, pin its upstream revision and notices, then prove a silent boot and a low-cost oscillator before implementing the capture path. See [the roadmap](docs/ROADMAP.md).
