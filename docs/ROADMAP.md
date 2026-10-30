# Roadmap

## Phase 0 — repository and platform audit

- [x] Establish a standalone project identity and document KAWA's reference constraints.
- [ ] Decide which platform components can be reused, record exact origins and notices, and confirm compatible packaging.
- [ ] Create a minimal build that boots and produces silence on target; preserve recovery and safe update behavior.
- [ ] Record available RAM and worst-case audio-block cycles.

## Phase 1 — playable sound core

- [ ] One integer oscillator/noise source with note-key control and a master level.
- [ ] Stable stereo output and panic/stop control.
- [ ] Host-side deterministic audio harness and sample/block budget checks.
- [ ] Verify FM-1 output and control mapping on physical hardware.

## Phase 2 — internal capture and basic glitches

- [ ] Fixed-size capture ring with explicit memory accounting and wrap-safe reads.
- [ ] Quantized stutter and micro-loop with adjustable division and probability.
- [ ] Reverse, freeze, scratch gesture and tape-stop envelopes.
- [ ] Exercise short/long buffers, rapid control movement and capture reset safely.

## Phase 3 — source and destruction palette

- [ ] License-cleared bundled samples and a compact sample voice.
- [ ] Bit-depth reduction and sample-rate reduction.
- [ ] Ring modulation and bounded distortion.
- [ ] Compare CPU cost and audio quality, with per-effect bypass.

## Phase 4 — performance workflow

- [ ] Clear live pages for source, buffer, rhythm and effects.
- [ ] Pattern variation with probability, intensity, quantization and repeatable seed.
- [ ] Presets and project persistence after safe storage design is verified.
- [ ] User guide, release package, checksums and recovery instructions.

## Release gate

Do not describe a release as FM-1-compatible until it builds reproducibly with pinned upstream dependencies, passes host checks, installs and recovers safely, and runs sustained audio/control stress tests on the physical FM-1 without missed blocks, crashes or data loss. Publish complete corresponding source and third-party notices.
