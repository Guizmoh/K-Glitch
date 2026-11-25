# Percussion presets: FLEXOR's picks (in progress)

FLEXOR is choosing LOAF BREAKER's drum presets by ear, on listening pages rendered on the host (11 Nov 2026). The goal:
a bank of styled presets that makes the drums easy to use (BEGINNER: a preset and an FX; EXPERT: every setting).

## The listening pages (private Claude artifacts, each with its picks in its store `choix/flexor`)

| Page | What | URL |
| --- | --- | --- |
| Snares Gabber | the 4 snares of today, 5 proposed, x SOFT / HARD / FOLD / BITS; 20 BITS snares x 4 crush levels | https://claude.ai/artifact/KaLB66rNnrJ3ZHSAsTRmxQ |
| Snares Crossbreed | 16 DnB-style snares x SOFT / HARD / BITS / BITS + CLAP | https://claude.ai/artifact/DgmNuMFitWLFMwfJJ11wKV |
| Snares Industrial | 16 snares from a prototype engine (Deathmachine, The Outside Agency, Tripped) x DRY / ROOM / CRUSH / SMASH; then FLEXOR's 9 x SMASH / MORE SNARE / MORE CLAP / SMASH + CRUSH | https://claude.ai/artifact/JGWdBdnugDp6R8HM5raEmB |

All at 175 BPM (the gabber page's first set at 180), the FM-1's kick (ROTTERDAM; INDUSTRIAL for the industrial page).

## What FLEXOR likes (11 Nov 2026)

- The industrial direction: a body that drops in pitch (the "thwack") or mass, heavy drive, the tail lifted, a short
  gated room; crushed (5 bits, 14.7 kHz) above all. **Nothing metallic** (no ring: RUST, SCRAP, ANVIL ... left out).
- Snares kept: **SHRAPNEL MORE SNARE** (the favourite: "a good snare for gabber", not only behind a kick), GRINDER MORE
  SNARE; SMASH + CRUSH: PISTON, CONCRETE, SHRAPNEL, BUNKER, GRINDER; CONCRETE SMASH. From the gabber page: GABBER BITS.
- Some sound like claps (noise + room): **BUNKER MORE CLAP** and **FURNACE MORE CLAP** kept, for the CLAP voice.

## The prototype engine (scratch render4.c / render5.c, not in the firmware)

Per sample: a body of three sines (f, 1.62 f, 2.31 f) whose pitch starts at f x (1 + DEPTH) and falls back over DROP ms;
band noise (one-pole high-pass then low-pass) and a 4 ms crack; the drive SOFT x/(1+|x|), HARD clip or FOLD (triangle);
LIFT: an envelope follower (fast up, slow down), gain (0.25 / env)^amount clamped 0.3..6; BITS: 16 levels a half, held
3 samples; ROOM: Schroeder (4 combs 1310-1927 samples, feedback 0.82 with a damping, 2 allpasses), gated (open
ROOM ms after the hit, then closed in ~6 ms), mixed in.

| Snare | f Hz | depth | drop ms | body dec | noise dec | noise band Hz | body | noise | crack | drive | gain | lift |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| GRINDER | 190 | 2.0 | 25 | 120 | 220 | 1500-7000 | .6 | .8 | .5 | HARD | 6 | .4 |
| FURNACE | 160 | 3.0 | 40 | 160 | 300 | 800-5000 | .7 | .7 | .3 | FOLD | 3 | .5 |
| PISTON | 180 | 4.0 | 20 | 110 | 160 | 1200-6000 | .9 | .5 | .8 | HARD | 8 | .2 |
| CONCRETE | 150 | 2.0 | 30 | 140 | 260 | 600-4000 | .8 | .6 | .3 | SOFT | 4 | .6 |
| SHRAPNEL | 260 | 3.0 | 6 | 50 | 120 | 3000-14000 | .5 | .9 | 1.0 | FOLD | 6 | .2 |
| BUNKER | 170 | 2.0 | 35 | 180 | 400 | 700-3500 | .7 | .7 | .3 | HARD | 4 | .7 |

The treatments: SMASH = gain x2, lift +0.35, room .45 for 280 ms; MORE SNARE = SMASH with body x1.6, noise x0.55, depth
x1.3, lift +0.15, room .30 for 150 ms; MORE CLAP = SMASH with body x0.45, noise x1.4 and three bursts 9 ms apart at the
start; CRUSH = BITS before the room. Each snare is levelled to the same loudness (its first 150 ms at about -15 dB RMS).

## In the firmware (0.225)

The engine has the drop, DIRT, LIFT and a compact gated ROOM (perc.c); the picks are presets (ui/perc.c PP_SN, PP_CP)
with the FX they were chosen with. They are approximations of the prototype (two sines, not three; the engine's own
noise): to be heard again on the FM-1 and tuned by ear.

## Next

1. The other voices' sounds (claps, hats) the same way, if FLEXOR wants.
2. The engine on a branch: the snare (and the clap) get DROP / DROP TIME, DIRT (SOFT, HARD, FOLD, BITS), LIFT, ROOM (a
   smaller gated room: RAM is tight, ~11 KB left in the pool), checked by ear against the prototype; then the picks as
   presets, rendered again by the firmware itself.
3. BEGINNER: each drum's page only its PRESET and an FX (as the pages' columns: DRY, ROOM, CRUSH, SMASH); EXPERT: all.
