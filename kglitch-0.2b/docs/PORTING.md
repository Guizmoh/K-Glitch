# Porting LOAF BREAKER to other hardware

LOAF BREAKER is written for the M-VAVE FM-1, but most of it never touches the FM-1. This file says what would move
as it is, what a new board has to provide, what would have to be redone, and in which order, for the day a more
powerful (open) instrument is chosen. Measured on 0.207 (10 Nov 2026).

## In one paragraph

The same code already runs on three targets: the FM-1, a desktop (the host simulator that runs every test and takes
every screenshot: `base/host/plat_host.c`) and a browser (the web emulator on the site). The sound engine, `app/dsp/`,
calls nothing of the platform (no `plat_*`, no libc beyond `<stdint.h>`: a project rule). The app (`app/app.c` and
`app/ui/*.c`) calls **31 platform functions** out of fm1-base's 144, and draws through two (`lcd_fill`, `lcd_blit`).
A port is a new implementation of those, plus whatever the new board's screen and controls ask of the layout.

## What moves as it is

| Part | Where | Notes |
| --- | --- | --- |
| The engine: break player, slicer, FX chain, kick (KICK LAB included), drums, melody, overlay | `app/dsp/*.c` | Plain C99, single-precision float. Builds with any `cc` (`tests/engine/run.sh` does it) |
| The engine tests | `tests/engine/` | Run anywhere: the first check on a new board's compiler |
| The app: settings, patterns, tracks, MIDI, the render, every page | `app/app.c`, `app/ui/*.c` (one unity build) | Moves as long as the platform functions below exist |
| The scenarios (scripted button presses, expectations, screenshots) | `tests/scenarios/*.txt` | Run on the host simulator: they keep checking the app while the port is built |
| The breaks | `breaks/*.wav`, `breaks/breaks.txt`, `tools/prep_breaks.py` | Packed into a bank (ADPCM) at build time; the packer is plain Python |

## What a new board has to provide

### The app's entry points (the platform calls these)

| Function | When |
| --- | --- |
| `app_init()` | Boot: the engine, then the saved settings |
| `app_ui_init()` | The first screen |
| `app_render(int32_t *lr, uint32_t n, uint32_t master_q12)` | The audio interrupt: `n` stereo frames (256 on the FM-1), interleaved, 24-bit in `int32_t`; `master_q12` the MASTER pot (4096 = full) |
| `app_frame()` | The UI: input and a redraw, about 60 times a second |
| `app_input_only()` | Between frames: input only |
| `app_panic()` | Silence now (before a firmware update) |

`BRK_SR` (`app/dsp/brk.h`) is 44,100 Hz; the render is built around 256-frame blocks (5.8 ms each). A board with a
different block size can call `app_render` in 256-frame pieces.

### The 31 platform functions LOAF BREAKER calls (`base/include/plat.h`)

| Group | Functions | What they do |
| --- | --- | --- |
| Time | `plat_ms`, `plat_ticks24` | Milliseconds since boot; a free-running 24 MHz count (`PLAT_TICKS_PER_US`): MIDI clock stamps, the CPU meter |
| Controls | `plat_keys`, `plat_buttons`, `plat_enc` | Keys held (a bit each, 27), buttons held (`B_*`, 14), encoder detents since the last call (`EN_*`, 7) |
| Lights | `plat_leds`, `plat_lights`, `plat_lights_mid`, `plat_glow` | Each button's and key's LED: on, dim, mid; the glow |
| MIDI | `plat_midi_in`, `plat_midi_in_t24`, `plat_midi_send`, `plat_midi_ago` | USB-MIDI packets in (with their port and time stamp) and out; the time since the last one |
| Storage | `plat_store_load`, `plat_store_save`, `plat_store_size`, `plat_store_head` | Numbered objects (`OBJ_*` in `app/app.h`) of at most 3,840 bytes: settings, patterns, steps, the KICK LAB… |
| Status | `plat_cpu_pct`, `plat_battery` | The render's CPU share (SETUP's METER); the battery |
| Bluetooth | `plat_ble_*`, `plat_bta_*` (13) | **Not to port**: LOAF BREAKER has no Bluetooth (FLEXOR's rule; `ble = false` in `app/fm1.toml`). Stub them or remove `app/ui/bta.c` |

So, without Bluetooth: **18 functions**.

### The screen

- Drawing goes through `lcd_fill(x, y, w, h, colour)` and `lcd_blit(x, y, w, h, pixels)` (RGB565), and fm1-base's
  canvas on top (`base/device/gfx.c`: `cv_begin`, `cv_rect`, `cv_line`, `cv_text`, `cv_text2`, `cv_blit`…), with
  Felucca's fonts (`FONT_T`, `FONT_XS`, `FONT_B`).
- The app draws a **240 × 240** screen in strips of at most 240 × 60 (`APP_CANVAS_PX` in `app/app.h`: the FM-1's RAM).
  With more RAM, raise it; nothing else needs to change.
- Every page's layout uses fixed coordinates (`app/ui/*.c`). Three ways to a new screen:
  1. **A 240 × 240 screen**: nothing to do.
  2. **A bigger square screen (480 × 480, 720 × 720)**: scale in the board's `lcd_fill` / `lcd_blit` (each pixel
     2 × 2 or 3 × 3). No UI change, and the cheapest by far.
  3. **Another shape (320 × 240, 800 × 480…)**: centre the 240 × 240 picture first (it works at once), then redo the
     layouts page by page if the room is wanted.

### The controls

LOAF BREAKER expects the FM-1's panel: **27 keys** (16 white, 11 black: `PLAT_WHITE_KEYS`, `PLAT_BLACK_KEYS`),
**14 buttons** (`B_FX`, `B_SEL`, `B_ENV`, `B_LFO`, `B_EDIT`, `B_GLO`, `B_HOME`, `B_SAVE`, `B_ARP`, `B_SEQ`, `B_PLAY`,
`B_REC`, `B_OCTDN`, `B_OCTUP`), **7 encoders** (`EN_SELECT`, `EN_ALGO`, `EN_PRESET`, `EN_K1`…`EN_K4`), the MASTER
pot, and an LED under each key and button. A board with the same counts needs only its own wiring map. One with fewer
needs its shortcuts remapped: they are all in `app/ui/main.c` (the button and knob handlers) and documented in
`app/docs/MANUAL.md` and `app/site/controlmap.html`.

### Storage

Any key-value store will do: flash pages, files on an SD card. Each object is read whole and written whole. On the
FM-1 a flash write stalls the audio, so LOAF BREAKER saves only on SAVE or after 10 s of silence; a board with
non-blocking storage can relax that.

## What stays behind

- **Bluetooth**, all of it (see above).
- The FM-1's update path: the loader's screen (`app/loader/`), the `.fwsc` package, the web installer, the package
  identity (`id_prefix` in `app/fm1.toml`), the Firmware Hub card. A new board brings its own way to install.
- The FM-1's memory map (`APP_STORE_MAP` in `app/app.h`, the XDATA sectors) and its section attributes. Define these
  for the new compiler: `PLAT_POOL` (big zeroed buffers), `BRK_RAM` (code run from RAM), `BRK_COLD` (code built for
  size); on a host they are empty or plain `noinline`.

## The FM-1's limits a stronger board would lift

| Limit today | Where it shows | With more room |
| --- | --- | --- |
| ~16 KB of flash left (565,800 of 581,564 B) | Every feature needs another taken out | Room for features again; more breaks (`IMAGE_CAPACITY`, 16 bank slots) |
| The bank built into the firmware, no samples of your own | The breaks are fixed at build time | Breaks loaded from storage, your own samples back |
| 5.8 ms per block, `APP_OVERLOAD` shedding FX | STACK doubles the kick's cost | Oversampled distortion (DISTO, SCREAM: less aliasing, cleaner gabber kicks), more voices, stereo FX |
| 96 KB RAM + 336 KB pool | The canvas in strips; one break decoded at a time (`CH_PCM_MAX`) | A whole-screen canvas, longer waves, several breaks in RAM |
| Single precision, no 64-bit division (the device's libgcc) | Rules in `CLAUDE.md` | Can stay as they are: they cost nothing on a bigger chip |

## A checklist for choosing the hardware

- A CPU with a **hardware single-precision FPU** (the engine is all `float`), ideally 200 MHz or more.
- **RAM**: at least 512 KB (the FM-1 uses ~420 KB); more for the gains above.
- **Flash**: 1 MB or more for the code and the bank, or storage for the breaks.
- **Audio**: a stereo codec at 44.1 kHz, a callback with blocks of 256 frames (or a multiple).
- **USB-MIDI** device (and a TRS MIDI in for the clock, as the FM-1).
- **A colour screen**: 240 × 240, or a square multiple of it (see "The screen").
- **Controls**: 27 keys, 14 buttons, 7 encoders and LEDs, or the budget to remap.
- **An open toolchain** (GCC or Clang) and a licence that lets the port be published (MPL-2.0, below).

Boards people build instruments on (to check against the list when the time comes, not verified here): the
Electrosmith Daisy (Cortex-M7, made for audio), PJRC's Teensy 4.x (Cortex-M7), boards on Raspberry Pi's RP2350, and
Linux boards (a Raspberry Pi; projects such as Zynthian). A Linux board is the shortest road: the host simulator is
already a Linux program.

## The order of the work

1. **Build the engine** with the board's compiler: `app/dsp/*.c` and `tests/engine/test_engine.c`. Run the engine
   tests on the board (or its simulator) and compare its render times with the host's.
2. **Copy the host platform** (`base/host/plat_host.c`, `base/host/build_host.sh`) as `plat_<board>.c`: it already
   implements every function above against a script and a framebuffer. Replace them one by one with the board's.
3. **Sound**: call `app_render` from the board's audio callback; check it stays well under its block's time.
4. **Screen**: `lcd_fill`, `lcd_blit` (and the scaling of option 2 above if the screen is bigger).
5. **Controls and lights**: the wiring map to `plat_keys`, `plat_buttons`, `plat_enc`, the LEDs.
6. **Storage**: the four `plat_store_*`.
7. **MIDI**: USB-MIDI in and out, then the TRS clock in.
8. **The breaks**: run `tools/prep_breaks.py` for the board's memory and build the bank in.
9. **The scenarios**: keep running `tests/scenarios/` on the host the whole time. On the board, play through
   `tests/scenarios/smoke.txt` by hand as a first check.

## Licences

LOAF BREAKER is FLEXOR's fork of CHOPPAH (Laurens Profittlich), built on fm1-base (Charles Vestal) and felucca-bsp
(Leo Kuroshita), all **MPL-2.0** file by file (`base/LICENSING.md`). A port may be published and changed: publish the
changed files under MPL-2.0 and keep the credits (CHOPPAH, fm1-base, Felucca). Code written new for the port can be
under any licence. The breaks are samples of records: their use is a separate question from the code's.
