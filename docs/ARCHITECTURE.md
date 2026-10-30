# K-GLITCH architecture

## Firmware base

K-GLITCH uses the KAWA FM-1 firmware as its starting point. This is an intentional GPL-3.0-only derivative; source files retain upstream notices and `LICENSING.md` records which portions came from Omnipotard, Leo Kuroshita / Hügelton Instruments, SLOOP and Dinghy. The imported source revision is recorded in the root README.

KAWA already supplies a standalone six-track sound engine (pulse, 4-bit waves, noise, six-operator FM, synthesized drums and user samples), sequencer, live controls and DSP. No external line input is involved in the core sound path.

## Internal capture and glitch path

`source/firmware/src/kawa_fx.c` captures the selected internal live bus in a 32,768-frame circular buffer. At 44.1 kHz this is about 0.743 seconds. Stereo signed 16-bit samples are packed in a 32-bit word, using 128 KiB for the ring. The audio callback reads the same buffer for synced loop and stutter, scratch, reverse, freeze, tape-stop, dive and roll effects. The capture is produced by the FM-1's sound engines and sequencer, not by an input jack.

Other effects include bit crushing, sample-rate reduction, filters, ring modulation, gating, distortion, echo, reverb and vinyl-style noise. A buffer effect can run with the existing synthesis and sequencer; the intended K-GLITCH direction is to make these transformations easier to perform and to add bounded autonomous rhythmic variation.

## Real-time and UI split

- **Audio interrupt:** process fixed-size chunks, synthesize voices, update the capture ring and effects, then write stereo output. KAWA keeps I/O, storage and panel drawing outside this path.
- **Main loop:** scan keys and encoders, update the tracker, edit parameters and publish event/control state for the interrupt.
- **Live panel:** retain the FM-1's screen, keys, buttons and encoders as a hands-on performance surface. K-GLITCH-specific page and gesture changes remain to be designed.

KAWA documents 44.1 kHz stereo output in 256-frame I2S halves (about 5.8 ms per half), an integer-only target without an FPU, and build-time image/RAM/pool checks. These are inherited source/build constraints; they do not demonstrate that additional DSP or buffer sizes fit K-GLITCH's eventual budget.

## Constraints and measurements still required

- The current 128 KiB ring already consumes a substantial fixed memory allocation. Measure free RAM on the actual target before enlarging it or adding voices/state.
- Profile worst-case cycles per audio block with multiple voices, each effect, USB traffic and display activity. Preserve margin below the interrupt deadline.
- Check clipping, wraparound, rapid effect changes and random-event density with host-side stress checks and on-device listening.
- K-GLITCH currently selects provisional identity `FM-1_957` (0.7) in its own build tree. Confirm that this identity is unused in the user's firmware releases and test recovery/update before any installation. The inherited KAWA build scheme has a constrained `FM-1_95N` / `FM-1_96N` identity range.
- No physical FM-1 build or validation is claimed yet.
