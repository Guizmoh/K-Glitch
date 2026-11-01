# K-GLITCH roadmap

## Base brought forward from KAWA

- [x] FM-1 platform: display, keys, encoders, USB-MIDI, audio output, recovery and build checks.
- [x] Internal synthesis and sample sources with six tracks and a step/song sequencer.
- [x] Internal stereo capture ring (32,768 frames, about 0.74 s at 44.1 kHz).
- [x] Live loop/stutter, scratch, reverse, freeze, tape-stop, dive and roll effects, plus crush, decimation, ring modulation and distortion.
- [x] Preserve KAWA's GPL license and attribution, plus the platform and third-party notices.

These are inherited KAWA capabilities; their presence does not mean K-GLITCH has been hardware-tested.

## Phase 1 — establish a clean K-GLITCH build

- [ ] Rebrand the remaining user-visible KAWA labels and generated package names.
- [ ] Assign and verify a package identity that cannot be confused with a KAWA or K-WASUB release.
- [ ] Run the host simulator, stress, UI, feature, format and loader checks.
- [ ] Build the FM-1 package with pinned JieLi toolchain and AC79 SDK dependencies.
- [ ] Measure image, pool, RAM and worst-case audio-block time; run on the physical FM-1.

## Phase 2 — autonomous performance layer

- [x] Add a bounded auto-glitch sequencer for stutter, loop, reverse, freeze and tape-stop gestures (first pass: one existing buffer effect at a time, one sequencer row per decision).
- [ ] Expose density, probability, intensity, quantization and variation rate as live controls.
- [ ] Add a repeatable random seed and musical constraints, with immediate hold/bypass/stop controls.
- [ ] Verify the automated gestures do not starve synthesis, exceed buffer history or create unsafe audio peaks on host and target hardware.

## Phase 2b — MPC REMOTE / HYBRID (one firmware, no reflash)

- [x] FX page TARGET: `MPC` (effects go out as MIDI CC only) and `MAST+MPC` (FM-1 master + CC), live state only.
- [x] Keys, LIVE lane and AUTO GLITCH mirrored as CC 127/0 on channel 16; nothing left stuck when leaving MPC (host test `source/tests/mpc.txt`).
- [ ] Continuous CCs from the knobs (pitch, filter, stutter rate, amount) and slice notes to trigger MPC pads.
- [ ] User-editable channel/CC map stored with the settings (several MPC profiles).
- [ ] Test with an MPC Live III over USB host (enumeration, latency, MIDI Learn).

## Phase 3 — focused instrument interface

- [ ] Design dedicated PLAY, BUFFER, GLITCH and FX pages using the FM-1's existing panel.
- [ ] Keep the tracker and sound editing available as an advanced mode while making the glitch performance path direct.
- [ ] Add useful presets and clear save/restore behavior after storage compatibility is reviewed.
- [ ] Document the controls in French and English and prepare safe install/recovery instructions.

## Release gate

Do not publish an installable K-GLITCH firmware until the unique package identity is confirmed, the full corresponding source and notices are provided, automated checks pass, and sustained audio/control/update recovery are verified on real hardware.
