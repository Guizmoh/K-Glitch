# K-GLITCH

K-GLITCH is an FM-1 glitch instrument project based on the KAWA tracker firmware. The project is now a **working source-code fork**, not yet a separately validated firmware release.

## What the KAWA base already gives us

The imported firmware has six tracks with pulse, wavetable, noise, FM, drum and sample sound engines; a step/song sequencer; a live FX page; and a 32,768-frame stereo capture ring. At KAWA's 44.1 kHz target rate that ring holds about 0.74 seconds of the internally generated mix. Its buffer effects include synced loops and stutters, scratch, reverse, freeze and tape-stop. Other live effects include bit crushing, sample-rate reduction, ring modulation and distortion. These functions operate on the FM-1's own output mix; an external line input is not required.

K-GLITCH now adds a first autonomous performance gesture on the FX page: turn KNOB 2 (the second encoder) to toggle AUTO GLITCH. While the sequencer plays, the firmware makes a 55% per-step chance to choose one internally buffered loop, stutter, scratch, reverse, roll, tape-stop, dive or freeze gesture. It is quantized to sequencer rows and uses the existing audio capture and effect engine; turning AUTO off or stopping playback releases it. The gesture state is live-only and is not stored in a project. Density/intensity controls, a dedicated performance page and hardware tuning remain planned.

## One firmware: STANDALONE, MPC REMOTE, HYBRID

On the FX (LIVE) page, KNOB 4 (TARGET) now goes past the tracks to two more choices. **MPC** sends the live effects only as USB-MIDI CCs (channel 16, 127 held / 0 released) to drive an MPC through MIDI Learn. **MAST+MPC** plays them on the FM-1 and sends them at the same time. Held keys, the sequencer's LIVE lane and AUTO GLITCH all go out. The table of numbers and the MPC setup are in [`docs/MPC_REMOTE.md`](docs/MPC_REMOTE.md). Tested in the simulator only.

## Build and development

See [`source/BUILDING.md`](source/BUILDING.md) for the upstream host simulator, browser emulator and FM-1 package builds. The FM-1 package build requires the JieLi pi32v2 toolchain and the specified AC79 SDK files. This checkout was not built or validated on an FM-1 during the bootstrap.

`source/` is imported from KAWA commit [`958901e`](https://github.com/Guizmoh/Kawa-Tracker-Fm1/commit/958901e) with its existing source layout and build tooling. K-GLITCH changes the displayed product name, output package name and development package identity; firmware modules retain their KAWA file names to keep the inherited build working. The provisional package identity must be confirmed before any hardware installation.

## Licensing and credits

K-GLITCH retains KAWA's GPL-3.0-only license and the upstream [`LICENSING.md`](LICENSING.md), which identifies KAWA contributions by Omnipotard and the separate platform contributions by Leo Kuroshita / Hügelton Instruments, SLOOP and Dinghy. Third-party notices and font licenses remain under `source/`. Do not remove or replace those notices when adapting the firmware. The KAWA and K-WASUB repositories are reference projects; only KAWA source is included here.

This project is not affiliated with or endorsed by M-VAVE or Hügelton Instruments.

## Status

- [Architecture and hardware constraints](docs/ARCHITECTURE.md)
- [Feature roadmap](docs/ROADMAP.md)
- [Upstream KAWA licensing and credits](LICENSING.md)
- FM-1 CPU, free RAM, interrupt headroom, package identity and physical controls still need confirmation.
- AUTO GLITCH and MPC REMOTE/HYBRID are implemented in source and tested in the host simulator; they have not been compiled for the FM-1 or validated on hardware or with an MPC.
- [MPC REMOTE / HYBRID: CC table and setup](docs/MPC_REMOTE.md)
