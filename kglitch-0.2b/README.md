# K-GLITCH 0.2b

**K-GLITCH 0.2b** (Guizmoh) is a fork of [LOAF BREAKER](https://github.com/FLEXOR-R/loaf-breaker) by FLEXOR, itself built
on [CHOPPAH](https://github.com/profikid/fm1-break-choppah) by Laurens Profittlich and
[fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0, credited below). It adds K-GLITCH's sixteen CHOP effects
(app/dsp/brk.c `kg_apply`), the GLITCH page (app/ui/kgchop.c: the chop editor and its 64-step lane, where the snare,
hat and clap settings were) and HOME's OCT- / OCT+ SHIFTs (OCT- + a white key: a CHOP effect, OCT+ + a white key: an ARP
effect). The installer: `../installer-0.2b/`. Build: fm1-base at 317cb86 in `base/` (see scripts/bootstrap.sh), then
`./build.sh --release 0.2`; `python3 tools/make_installer.py app/build/k-glitch-0.2.fwsc 0.2b OUT.html`.

---

<p align="center">
<picture>
<source media="(prefers-color-scheme: light)" srcset="docs/img/banner-light.png">
<img src="docs/img/banner.png" alt="LOAF BREAKER" width="340">
</picture>
</p>

<h1 align="center">LOAF BREAKER</h1>
<p align="center"><b>A hardcore kick synthesizer powerhouse and a breakbeat chopper for the M-VAVE FM-1.</b><br>HARDCORE - GABBER - BREAKCORE - TERROR - SPEEDCORE</p>

<p align="center">
<a href="https://flexor-r.github.io/loaf-breaker/"><b>Site</b></a> ·
<a href="https://flexor-r.github.io/loaf-breaker/emu/">Emulator</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/install/">Install</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/app/">App (Android)</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/manual/">Manual</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/manual/#controls">Controls</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/breaks/">Breaks</a> ·
<a href="https://flexor-r.github.io/loaf-breaker/import/">Import</a> ·
<a href="../../releases">Releases</a><br>
<a href="https://flexor-r.github.io/loaf-breaker/manual/#start-in-five-minutes"><b>Start in five minutes</b></a>
</p>

> **A work in progress, on purpose.** LOAF BREAKER changes all the time: new versions come out often, and each can
> move things. The built-in breaks are not final: they will be swapped for better ones as they are sampled (the breaks
> you send from the site's Breaks page stay yours). Pages of the firmware and of the site may still change their layout
> or their look, and some sections will be redesigned. The aim is a machine that is quick to play live, easy to find
> your way in, and yours to shape: if you are here, you are early, and the best is still to come.

LOAF BREAKER is FLEXOR's firmware for the [M-VAVE FM-1](https://www.m-vave.com/), built on
**[CHOPPAH](https://github.com/profikid/fm1-break-choppah)**, Laurens Profittlich's Amen chopper. It keeps CHOPPAH's
heart (a break cut into sixteen slices on the white keys, effect pads on the black keys, a generator, a master effects
chain, patterns: 64 of them here) and drives it into hardcore and breakcore:

- **The kick first.** Its own menu, its own synth (pitch drop, hold, rumble, tune), **DISTO**, **SCREAM** and
  **DISTO +**, a **GLIDE** between notes and the rawstyle **REVERSE BASS**, its own EQ and FX (**MAGIC EQ**, **MAGIC MBC**),
  twenty memories, a note on every step, and its real sound drawn live on screen. The **KICK LAB**: a CLEAN tail (one
  note from the hit to the end, no "talking" distortion), its NOTE, the pitch drop's CURVE, END, a closing TAIL
  FILTER and DRIVE, **STACK** (another kick's attack on this one's tail), MUTATE and A/B.
- **A sequencer for live.** Kick, snare, hat and clap over 16, 32 or 64 steps; per step **NOTE, HITS, COND** (chance, 1:2, 2:2) and
  **NUDGE**; **ARP: sixteen effects over everything** on the white keys (the ARP page).
- **Tracks and drops.** Patterns in order with loops and per-item breaks, a radio-style PLAY, builds that don't sound
  alike (snare or kick rolls, stutter, sweeps, a gap, the **tempo rising** to the drop).
- **A mix that holds.** COMP, DUCK, MAX OUT, rolls on the break or the kick. Kicks and drums only: no melody (gone in 0.209).
- **A screen that moves.** The mix scrolling as a CDJ in rekordbox colours, the track's progress and "DROP IN 4",
  beat-lit tempo, kick flashes, screen effects (CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT), a glitched start-up.

Built on [fm1-base](https://github.com/charlesvestal/fm1-base) (MPL-2.0). No Bluetooth: MIDI over USB and the TRS jack.

![LOAF BREAKER's screens: the splash, the start screen, playing (the CDJ view), the slices, a key held, the KICK menu's RGB view](docs/img/screens.png)

![Holding a pad shows its effect and puts its settings on K1-K4: roll, gate, freeze, filter throw, vinyl break, the ARP key's overlay, kick triplets; ARP › BLACK, the keys' layout](docs/img/pads.png)

![The kick: the built-in kicks (PRESET), the TOK attack, DIRT (DISTO), the clean tail, the snare, the kick's FX1](docs/img/kick.png)

![Live: TRACKS, PLAY (the radio), a build before the drop, a step's HITS, the MASTER (COMP, DUCK, MAX OUT), the pattern](docs/img/live.png)

![The newest menus: ARP's sixteen effects (each on the break or the kick), SEL BLACK, the kick's TAIL MODE SPLIT, a white key's LENGTH (each break its own), SONG › BUILD BRK, AFTER DROP SAME, MIX › ROUTING, the FX page's FX MIX, SYSTEM › DISPLAY, RESET, the kick's tail GATE, SYSTEM › LIGHTS](docs/img/menus.png)

- **The manual**: [online](https://flexor-r.github.io/loaf-breaker/manual/), or [app/docs/MANUAL.md](app/docs/MANUAL.md)
- **Download the firmware**: [releases](../../releases) (`loaf-breaker-A.B.C.fwsc`, e.g. `loaf-breaker-0.9.76.fwsc`), each release also carries
  the site (emulator, installer, manual) as `loaf-breaker-site-A.B.C.zip`.
- **Install**: fm1-base's web installer (Chrome or Edge, the FM-1 plugged in), or
  `python3 base/tools/fm1_install.py loaf-breaker-A.B.C.fwsc`

Installing is at your own risk; the installer can put M-VAVE's own firmware back, and an FM-1 that fails to start
twice comes up in a safe mode the installer can still reach. If your FM-1 runs M-VAVE's FM1 Open firmware, restore
M-VAVE's original firmware first ([docs/FLASHING.md](docs/FLASHING.md)).

**Status: 0.9.x beta, on its way to 1.0.** Every push to the `loaf` branch is a release (`loaf-vA.B.C`). LOAF BREAKER has no Bluetooth.

## Panel

Each button is a domain, named on the screen: LIVE (HOME), SOUNDS (EDIT), PATTERN (SEQ), SONG (ENV), MIX (LFO), EFFECTS (FX),
SYSTEM (GLO). The button again: the next part; SEL + a button: that domain's second function (tap SEL to see them all).

| Control | Does |
| --- | --- |
| White keys 1–16 | Slices: at the next snap point while playing, at once while stopped. **Hold to loop**: K1–K4 set that key's loop (LENGTH, PITCH, MODE, DECAY; each break its own). **Key held + FX**: that slice's own effect |
| Black keys | Pads, while held, on a layout (EXPERT: ARP › BLACK). CHOPPAH's, a new FM-1's (BEGINNER keeps the layout EXPERT left): **R8 · R16 · R32** rolls (K2 THRU: the break, the kick or both; SEL + a roll pad: a kick roll for that press) · **REV** reverse · **FRZ** freeze · **HLF** half speed · **VNL** vinyl break · **GAT** gate · **RND** random (REV, FRZ, HLF, GAT, RND each with a LATCH: a press on, the next off) · **FLT** filter throw (K4 THRU: the break, the drums or both) · **DUB** dub echo throw. FLEXOR LAYOUT: R8 R16 R32 ARP ARP TRI X4 X2 SUB DST KIL: two **ARP** keys (an overlay effect each, the second on RISE), **TRI** · **X4** · **X2** the kicks in triplets, on every step, doubled, **SUB** the kick's SUB FX off / on (its effect untouched), **DST** the kick's DISTO off / on (its amount untouched), **KIL** the sequencer's kicks quiet while held (or latched). CUSTOM: any of them on any key. K1–K4 tune the pad held |
| PLAY | Play / stop · HOME + PLAY: back to bar 1 |
| SELECT | Tempo · on the pages with tabs, the tabs at the top (the KICK menu's sections, EFFECTS' slots, SONG's tabs) and ALGORITHM those below · **SEL + SELECT**: the drums' filter (DRUM FLT) on any page |
| SEL | **Live: SEL + K1–K4 change the kick while it plays, on any page; SEL twice locks SEL down (lit), both hands free; once more frees it.** **Tapped alone**: the shortcuts card, those that work everywhere and this page's own · **SEL ×2**: locked down, lit, for the combinations one-handed (press again: free); held or locked, the foot shows SEL + K1–K4's functions and values · **SEL + K1–K4**: four live functions you assign on EDIT › KNOBS (or all four at once on ASSIGN with a style: TERROR, EARLY NEXT GEN, HARDCORE …, or one of your own PRESET 1–4, their values too: SAVE on it stores them), each kick its own (PITCH DROP, RUMBLE, PUNCH, DRIVE, DISTO, SCREAM, DISTO +, its TYPE, SCREECH, TAIL…) · **SEL + REC** (on FX): the effect under your hands back to its start (again within 4 s: undo) · **SEL + ENV**: the drop · **SEL + ALGORITHM** the break (where ALGORITHM is already the break, HOME, BREAK, SONG, MIX, ARP: BREAK TIME NORMAL / HALF), **SEL + PRESETS** the pattern (on HOME, PATTERN, MIX, ARP, SYSTEM's MIDI / SCREEN SET / RESET: the kick, KICK 1–20) · **SEL + SEQ** copy, then paste the pattern · **SEL + ARP** in the KICK menu: copy the kick, then paste it on another (a drum's page: the drum) · **SEL + SAVE** undo · SEL alone: every combination on the strip |
| HOME | The main page: PERFORM, GENERATOR, SPLIT (EXPERT: two breaks in one loop) and CHAOS (the break scrambled by role), with the **CDJ view** (the mix scrolling in, in WAVE TYPE's colours: RGB, SERATO, 3 BAND, BLUE; the playhead at the right edge); HOME held: the slices; the waveform's settings are SYSTEM's (GLO › DISPLAY: VIEW (waveform or slices), WAVE TYPE (RGB, SERATO, 3 BAND, BLUE), WAVE ZOOM, BREAK TIME (the break at half the tempo)) |
| EDIT | **SOUNDS**. The **KICK menu**, by the kick's anatomy: ATTACK (PRESET: the built-in kicks, where it opens; CLICK, PUNCH, STACK: another kick's attack), BODY (MIX, SUB, GRIT, SYNTH: DROP, HOLD, RUMBLE, ASYM, TUNE…, MACRO, OUT), TAIL (a CLEAN tail, its NOTE, CURVE, END, FILTER; BASS: GLIDE, the rawstyle REVERSE BASS; MORPH), DIRT (DISTO, SCREAM, DISTO +), FX1 and FX2 (its two FX slots, each kick its own), MORE (KNOBS: what SEL + K1–K4 turn, ASSIGN, TOOLS: MUTATE, A/B, EDIT VIEW, DRUM VIEW ZOOM; its EQ, EQ B and EQ S); twenty kicks, KICK 1–20 (PRESETS, on the KICK menu's pages; in its FX section PRESETS the effect, SEL + PRESETS the kick; SEL + PRESETS elsewhere a knob's function, a style), each naming the built-in it came from; the kick drawn in RGB as rekordbox or Serato do (SYSTEM › DISPLAY K2), whole or only the part edited (EDIT VIEW: KICK › MORE › TOOLS K3) (EDIT held: WAVE, CLASSIC); SELECT: the next section, ALGORITHM: the next part; EDIT again: the next sound (KICK, SNARE, HAT, CLAP); EDIT held: the classic view. **SEL + EDIT**: the next source, DRUMS (snare, hat, clap: EDIT again the next), BREAK (pitch, grain, snap, level; ALGORITHM the break) |
| SEQ | **PATTERN** (ALGORITHM every lane, top to bottom; SELECT the views FULL, DRUMS, GROOVE (SWING, BREAK ROWS, FOLLOW); SEQ again: HOME): the sequencer: the break's steps and the drum lanes, each drum over 16, 32 or 64 steps. **A step held**: K1 NOTE, K2 HITS (1–4 taps in the step), K3 COND (chance, 1:2, 2:2), K4 NUDGE. Playing, the keys follow the bar playing. **SEQ held** + key 1–4: copy the bar there; + K1: shift the lane a step; + REC: clear the drums; then SEQ again: clear the pattern. **REC** on a drum lane: tap it in |
| ENV | **SONG** (SELECT its tabs; ENV again: HOME): TRACKS (patterns, loops, drops and each item's own break and kick; a DROP its build's kick (K1), STUTTER (K3) and what its GAP keeps (K4: the break, the drums, the echoes); ENV held: a space after the item, or the space taken out), PLAY (a radio: tune a track, press PLAY), DROP, BUILD (the build before a drop: style, sweep, riser, a gap; PRESETS: the tempo rising to the drop), BUILD BRK (the break in the build: still or rolled), DROP BANK (DROP 1–8, each a whole drop) |
| LFO | **MIX** (SELECT its tabs; LFO again: HOME): LEVELS (break, kick, drums), ROUTING (FX: the FX fader; DRUMS: around the FX (the default) or through it; DUCKS: what DUCK pushes down, BREAK, PERC or BOTH; ROLL: what the rolls take; each ARP effect's THRU is on the ARP page), MASTER (COMP, RATIO, DUCK, MAX OUT), DYNAMICS (the compressor's and the ducker's timing) · **LFO held + K1–K4**: the four levels on any page |
| FX | **EFFECTS** (SELECT the slot, ALGORITHM its part MAIN / MORE / MOD; FX again: HOME): the master FX chain: four slots (FILTER, CRUNCH, DRIVE, PUNCH, FLANGER, RING MOD, TREMOLO, REVERB, DELAY, EQ, TAPE, MB COMP, CLIP); each effect remembers its settings when you change it and back; FLEXOR's presets MAGIC EQ and MAGIC MBC after CLIP (BEGINNER: two slots, a short list) |
| ARP | **ARP** (EXPERT: SELECT its three tabs, ARP, BLACK, SEL BLACK; ARP again: HOME): sixteen effects over everything, one a white key (held, or LATCH: on and off); K1 the amount, K2 THRU (BREAK, KICK or THRU), K3 RATE or SWEEP, K4 LATCH · SEL + ARP: PATTERN › GROOVE (SWING) |
| GLO | **SYSTEM** (SELECT its parts; GLO again: HOME): MIDI clock in and out (USB, TRS), and in EXPERT the METER; SETUP: 24 skins, KICK EQ (EXPERT), MODE (TUTORIAL: 30 guided steps, the buttons to press blinking; BEGINNER: the essentials (the KICK three pages, KICK, SHAPE and MIX; two FX slots and a short list; a new FM-1 asks for its mode first); EXPERT: all of it), AUTO SAVE; SCREEN FX: the screen effects, several at once (CRT, VHS, GRAIN, RGB, SHAKE, STROBE, 8-BIT), (the waves' colours, WAVE ZOOM and BREAK TIME: SYSTEM › DISPLAY; EDIT VIEW and DRUM VIEW ZOOM: KICK › MORE › TOOLS); SCREEN SET: their settings; LIGHTS: the glow and the pulse on the tempo (GLOW, PULSE, PULSE ON, STYLE); RESET: one part at a time, or, its last item, EVERYTHING (RESET ALL): K4 picks, K2 five clicks, then GLO within 5 s |
| SAVE | Keep the settings, the patterns, the tracks and the steps |

## Breaks

LOAF BREAKER has 8 breaks on ALGORITHM, 2 bars each, slowest first: AMEN10 135, AMEN05 158, AMEN20 164, UPDOWN 165,
AMEN06 169, AMEN19 172, AMEN13 173 and FOCUZ 175 (FLEXOR's choice, 0.9.70; FOCUZ from `breaks/flexor/`), every break
on an even grid and at the same level (-12 LUFS). [`breaks/breaks.txt`](breaks/breaks.txt) marks with `*` the ones
built in (the bank has 400 KB); `./build.sh` builds them into `app/bank_data.h` (generated, git-ignored). The breaks
are built in, and your own go after them, five to seven in 232 KB (0.9.32, from CHOPPAH 1.4's break manager): the site's
[Breaks](https://flexor-r.github.io/loaf-breaker/breaks/) page (`app/site/breaks.html`) plays the breaks of
`breaks/` (made by `tools/break_lib.py`) or a sample of yours as the FM-1 will, and writes your list over USB into two
places of the flash (140 KB and 92 KB, outside the saves), after the eight built in.
Your own snare, hat and clap too (0.9.45): the site's [Import](https://flexor-r.github.io/loaf-breaker/import/) page
(`app/site/import.html`) trims them, fits the three in the FM-1's 8 KB for them (22.05, 16 or 11.025 kHz, IMA ADPCM)
and writes them over USB; each drum's PRESET then has one more, the last: your sample.

## Building

```
scripts/bootstrap.sh    # once: fm1-base (submodule), JieLi toolchain, AC79 SDK files, Python packages
./build.sh test         # the engine's tests, plain cc, no toolchain needed
./build.sh              # app/build/loaf-breaker.fwsc (macOS: the toolchain runs in Docker / OrbStack)
./build.sh --host       # the host simulator: app/build/host/loaf-breaker_host tests/scenarios/ui.txt OUTDIR
./build.sh --web        # the browser emulator (Emscripten)
./build.sh flash        # install over USB-MIDI (docs/FLASHING.md)
```

Clone with `--recursive` (fm1-base is a submodule in `base/`).

| Path | What |
| --- | --- |
| `app/` | The firmware: `app.c` (screens, controls, glue), `app.h`, `fm1.toml` (build and site settings) |
| `app/dsp/` | The engine, platform-free: `brk` (sequencer, generator, pads, voices), `brk_fx`, `adpcm`, `brk_synth`, `brk_bank.h` |
| `app/docs/`, `app/site/` | The manual and the site's front page |
| `tests/engine/` | Engine tests (plain cc), with WAV renders |
| `tests/scenarios/` | Host-simulator scripts: keys in, audio and screenshots out |
| `breaks/` | The recorded breaks (WAV) and `breaks.txt`, which of them go into the firmware |
| `tools/prep_breaks.py` | WAV → slices → IMA ADPCM bank (`--list breaks/breaks.txt`, as `./build.sh` runs it) |
| `docs/` | [PRD](docs/PRD.md), [flashing and recovery](docs/FLASHING.md), [the FM-1 Firmware Hub's submission](docs/hub/SUBMIT.md), [porting to other hardware](docs/PORTING.md) |

## Publishing

Every push to the `loaf` branch is a release (`.github/workflows/loaf.yml`): the engine tests, the firmware and
its size report, the host scenarios, then a GitHub release `loaf-vA.B.C` (the last tag's patch + 1) with `loaf-breaker-A.B.C.fwsc` and the site
built from the same commit (front page, installer, emulator, manual with its screenshots: `loaf-breaker-site-A.B.C.zip`).
The package number fm1-base sees is A+2.B.C (0.9.76 is 2.9.76).

The manual's and the site's screenshots come from the host simulator: `./build.sh --host`, then
`sh tools/site_shots.sh` (into `app/docs/img`).

## Licence and credits

MPL-2.0 ([LICENSE](LICENSE)). LOAF BREAKER is built on [CHOPPAH](https://github.com/profikid/fm1-break-choppah) by
Laurens Profittlich, itself on [fm1-base](https://github.com/charlesvestal/fm1-base) and
[felucca-bsp](https://github.com/hugelton/felucca-bsp), from [Felucca](https://github.com/hugelton/Felucca) by Leo
Kuroshita (Hügelton Instruments).

M-VAVE and FM-1 are trademarks of their owners. LOAF BREAKER is not affiliated with M-VAVE, nor with CHOPPAH's author.
