# K-GLITCH architecture (proposal)

## Operating model

K-GLITCH is a standalone instrument. Sound originates in on-device synthesis and optional bundled samples. An internal post-source audio tap feeds a rolling capture ring. The player reads the ring at a separately controlled position/rate, making short repeats, reverse scans, freezes and scratch gestures possible without external audio input.

## Real-time audio graph

1. **Sources:** a small number of integer oscillators/noise voices and bounded sample voices.
2. **Mix/capture:** sum to a defined internal bus and write to a power-of-two ring. Capture must never wait on UI, storage or USB.
3. **Read head:** choose start, length, direction, playback ratio and crossfade. Clamp every read to valid captured history; no uninitialized memory is audible.
4. **Effects:** optional rate/bit reduction, ring modulation and saturating distortion. Keep state per effect small and avoid floating point.
5. **Output:** saturating stereo conversion with a reliable bypass/stop path.

The exact graph location and sample format depend on the selected platform audio API and available RAM. Effects should be processed in the existing audio callback/block; no heap allocation, file I/O, locks or unbounded loops belong in that callback. Control changes cross into audio through small, atomic or interrupt-safe parameter snapshots, subject to the platform's actual guarantees.

## FM-1 facts from the KAWA technical reference

KAWA's `source/firmware/src/platform.c` documents a 240x240 screen, 27 note keys, buttons and encoders, USB-MIDI, and class-compliant stereo USB audio. Its current target path uses 44.1 kHz and 256 stereo frames per I2S half-buffer, approximately 5.8 ms per half. Its build notes state that the target has no FPU and enforce image/RAM/pool limits. These are reference-repository observations, not a complete independent hardware specification and not a K-GLITCH validation.

At 44.1 kHz, a mono signed 16-bit capture ring would consume 88,200 bytes per second; stereo would consume 176,400 bytes per second, before bookkeeping or other firmware memory. A one-second stereo ring is therefore likely too costly until the usable RAM budget is measured. A compact mono or short stereo ring and sample-rate/bit-depth choices must be benchmarked on the real target.

## Thread and control split

- **Audio callback/interrupt:** read immutable control snapshot; render sources; write capture; read/process output; advance bounded state.
- **Main loop:** scan keys/encoders, update pages, quantize event requests, generate bounded pattern variation, publish parameter snapshots.
- **Storage/USB:** outside audio callback. Sample imports and presets are future work and must not stall audio.

## Playable live interface

Proposed pages: **PLAY** (source, tempo, pattern and master), **BUFFER** (length, capture tap, loop size, reverse/freeze), **GLITCH** (stutter division, probability, density, swing), and **FX** (crush, ring, drive). Dedicated hardware buttons should provide immediate hold/toggle gestures for freeze, stutter and panic/stop where the button map allows. Encoders set bounded parameters; note keys trigger source pitches or mapped slices. The exact mapping requires a physical-device interaction pass.

Randomization is seeded and constrained: probability controls whether a musical event changes; intensity limits the range; quantized mode changes only at musical boundaries; a clear bypass/stop remains available. Defaults should be rhythmic and safe, with chaotic behavior opt-in.

## Open platform questions / required measurements

- Identify the exact FM-1/JieLi target, SDK/toolchain revision, platform APIs and package identity allowed for a separate firmware.
- Measure free RAM after platform, UI and source state; calculate ring sizes from the measured budget.
- Measure worst-case cycles per 256-frame audio block for each source/effect combination, including UI and USB load; retain interrupt headroom.
- Verify callback concurrency, cache/alignment requirements, audio sample format, channel routing, clipping behavior and safe parameter handoff.
- Determine whether flash-backed samples can be streamed without audio stalls and what storage region is safe for a separate project.
- Test boot, recovery, USB audio, controls, sustained playback, buffer wrap, effect extremes, random event density and update recovery on physical hardware.

Until these questions are resolved, no CPU, RAM or latency budget is guaranteed.
