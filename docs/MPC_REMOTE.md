# K-GLITCH — MPC REMOTE / HYBRID

K-GLITCH is a single firmware with three ways to use the live effects. You switch between them on the **EFFECTS (LIVE)** page with **KNOB 4 (TARGET/CIBLE)**. You never reflash.

| TARGET | What happens |
| --- | --- |
| `MASTER`, `T1`…`T6` | **STANDALONE**: the effects play on the FM-1 (the master or one track). No MIDI CC is sent. |
| `MPC` | **MPC REMOTE**: the FM-1 plays no effect itself. Each effect is sent as a MIDI CC to the MPC. |
| `MAST+MPC` | **HYBRID**: the effects play on the FM-1 master **and** are sent as a CC to the MPC. |

Effects sent to the MPC include more than the held keys. The **LIVE** lane of the sequencer (live-effect recording, the effect sequencer) and **AUTO GLITCH** (KNOB 2) send them too. So the FM-1's sequencer can program or randomise effects on the MPC.

## Message format

- USB-MIDI, **channel 16**, Control Change.
- Value **127** while the effect is active, **0** when it stops (a "momentary" button in MIDI Learn).
- Moving TARGET away from MPC/MAST+MPC sends 0 to every CC that is still at 127. Nothing stays stuck.
- With AUTO GLITCH or the LIVE lane, the FM-1 sequencer has to be playing. To sync it to the MPC, set **OPTIONS → MIDI CLOCK: IN** (the MPC must send its clock).

## CC table

The numbers avoid bank select, modulation, volume, pan, sustain, the RPN/NRPN and the channel-mode messages.

| Key | POCKET bank | CC | WRECK bank | CC |
| --- | --- | --- | --- | --- |
| 1 | LOOP 1/16 | 20 | CRUSH 4 bits | 106 |
| 2 | LOOP 1/12 | 21 | LO-FI rate | 107 |
| 3 | LOOP 1/32 | 22 | LOWPASS sweep | 108 |
| 4 | LOOP 1/64 | 23 | HIPASS sweep | 109 |
| 5 | UNISON | 24 | TAPE STOP | 110 |
| 6 | UNISON LOW | 25 | DIVE | 111 |
| 7 | OCTAVE +1 | 26 | GATE 1/16 | 112 |
| 8 | OCTAVE −1 | 27 | GATE 1/32 | 113 |
| 9 | STUTTER x4 | 28 | FREEZE | 114 |
| 10 | STUTTER x3 | 29 | RING | 115 |
| 11 | SCRATCH | 30 | ECHO | 116 |
| 12 | SCRATCH FAST | 31 | DISTORT | 117 |
| 13 | 6/8 TRIPLET | 102 | WOBBLE | 118 |
| 14 | RETRIG 1 beat | 103 | VINYL | 119 |
| 15 | REVERSE | 104 | NO MELODY | 85 |
| 16 | BUILDUP ROLL | 105 | NO BEAT | 86 |

## MPC Live III setup (example)

1. Connect the FM-1 over USB to a USB-A host port on the MPC. In the MPC's MIDI preferences, enable the FM-1 as a MIDI input.
2. On a MIDI-learnable parameter (pad pitch, filter cutoff, an insert effect's bypass, a mute...), start **MIDI Learn**, then hold the matching FM-1 key.
3. Repeat for each effect you want to use. Save the MPC project. The mappings live on the MPC side.

On the MPC, the CC drives whatever you assign to it. The FM-1 sends 127/0 only; no CC sweeps or slice notes yet (see the roadmap).

## Status

Checked on the host simulator (`source/tests/mpc.txt`: keys, HYBRID, AUTO GLITCH, return to MASTER). **Not yet tested on a real FM-1 or with an MPC.**
