/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's kick (PRD K-*): a synthesised kick of four layers for gabber, hardcore and techno over the break.
 *   SUB    the low foundation: a sine dropping to its pitch, a long tail
 *   PUNCH  the hit and the body: a sine swept fast from far above (909 style)
 *   GRIT   the dirty mids: FM through a drive, noise mixed in by its tone
 *   CLICK  the transient: a short burst of noise and a click
 * Each layer has a level, pitch, decay and tone, and an effect of its own (drive, crunch, filter, ring mod) with a
 * wet mix; the layers sum into INTENSITY (a drive into a clipper: the distorted gabber kick). Platform-free like the
 * break engine. Settings from the main loop (kick_set, read once a block); triggers at sample offsets in the render. */
#pragma once
#include <stdint.h>

enum { KL_SUB, KL_PUNCH, KL_GRIT, KL_CLICK, KICK_LAYERS };
enum { KFX_OFF, KFX_DRIVE, KFX_CRUNCH, KFX_FILTER, KFX_RING, KICK_FX };
extern const char *const KICK_LAYER_NAME[KICK_LAYERS];
extern const char *const KICK_FX_NAME[KICK_FX];

typedef struct {
    float level;                         /* 0..1 */
    float pitch;                         /* 0..1: SUB 30..90 Hz, PUNCH 50..200 Hz, GRIT 40..400 Hz, CLICK 1..12 kHz */
    float decay;                         /* 0..1: to -40 dB in SUB 60..1500 ms, PUNCH 20..400, GRIT 20..800, CLICK
                                          * 1..40 */
    float tone;                          /* 0..1: SUB its saturation, PUNCH its sweep, GRIT FM depth and noise,
                                          * CLICK noise or click */
    uint8_t fx;                          /* KFX_* */
    uint8_t rsv[3];
    float fa, fb, wet;                   /* the effect's two settings and its mix, 0..1 */
} kick_layer_t;                          /* 32 bytes */
/* LOAF BREAKER: TERROR, after INTENSITY: the kick's mids and highs driven hard into a wavefolder and a clip, the sub
 * under them kept clean; 0 off .. 100. In the SUB layer's first spare byte (a kick saved before it: 0, off) */
#define KICK_TERROR(k) ((k)->l[KL_SUB].rsv[0])
/* LOAF BREAKER: TERROR 2, the violent one, after TERROR, 0 off .. 100: the whole kick driven up to ~80x through a
 * sine folder (it folds round and round: a screech), its mids pushed (~1-3 kHz, the scream), clipped hard; the sub
 * under ~110 Hz laid back under it, the folded part following the kick's level. In the PUNCH layer's first spare
 * byte (a kick saved before it: off) */
#define KICK_TERROR2(k) ((k)->l[KL_PUNCH].rsv[0])
/* LOAF BREAKER: their settings (ENV's KNOB), -50 .. +50 (int8; 0, a kick saved before them: as they always were),
 * in the layers' other spare bytes. TERROR: SUB (the split kept clean, ~48..250 Hz), FIZZ (its top, darker ..
 * brighter), SUSTAIN (the fold following the kick down .. a wall). TERROR 2: SUB, SCREAM (the mids pushed, ~150 Hz ..
 * 2.4 kHz), PUSH (how hard) */
#define KICK_T_SUB(k) ((k)->l[KL_SUB].rsv[1])
#define KICK_T_FIZZ(k) ((k)->l[KL_SUB].rsv[2])
#define KICK_T_SUS(k) ((k)->l[KL_GRIT].rsv[1])
#define KICK_T2_SUB(k) ((k)->l[KL_PUNCH].rsv[1])
#define KICK_T2_SCREAM(k) ((k)->l[KL_PUNCH].rsv[2])
#define KICK_T2_PUSH(k) ((k)->l[KL_CLICK].rsv[1])
/* LOAF BREAKER (TEST): TERROR's more: DRIVE (-50 .. +50, int8: the fold's gain apart from AMOUNT's mix), TYPE (0 FOLD,
 * 1 CLIP: squared off, 2 CRUSH: folded then stepped, 3 RECT: rectified, an octave up and hollow, 4 TUBE: uneven,
 * warm and fat), SCREECH (0 off, 1..100: a resonant band after it, ~500 Hz ..
 * 5 kHz: the whistle), TAIL (-50 .. +50: the distortion dying with the kick faster .. hanging on) */
#define KICK_T_DRIVE(k) ((k)->l[KL_GRIT].rsv[0])
#define KICK_T_TYPE(k) ((k)->l[KL_GRIT].rsv[2])
#define KICK_T_TYPES 9u                  /* (LOAF BREAKER: FOLD CLIP CRUSH RECT TUBE, 0.255 HALF SINE WRAP FUZZ) */
#define KICK_T_SCREECH(k) ((k)->l[KL_CLICK].rsv[0])
#define KICK_T_TAIL(k) ((k)->l[KL_CLICK].rsv[2])

typedef struct {
    kick_layer_t l[KICK_LAYERS];
    float intensity;                     /* 0..1: the sum's drive into the clipper (0 clean) */
    float volume;                        /* 0..1 */
} kick_t;                                /* 136 bytes (its steps are the pattern's: the app's) */

#define KICK_PRESETS 12u                /* (0.9.39: the table's; K1's round KICK_PRESET_ORD) */
#define KICK_PRESETS_ON 11u
#define KICK_PRESET_DISTORTO 5u          /* (0.9.39, FLEXOR: out of the round, kept for the kicks saved from it) */
#define KICK_PRESET_ROTTERDAM 3u
#define KICK_PRESET_TERROR 6u            /* (LOAF BREAKER, 0.9.32, FLEXOR: TERROR, his kick made a preset: ROTTERDAM, GRIT
                                          * 0, DISTO 30 FOLD, DRIVE 69, SUB 1279 ms, SUB FX DRIVE 50; with the app's part,
                                          * ui/kick.c kpre_extra: SYNTH DROP +20, TIME 167 ms, SHAPE -50, TAIL + DRIVE 100,
                                          * EQ LOW +6.2 dB) */
#define KICK_PRESET_SPINE 7u            /* (0.9.32, FLEXOR: SPINE, TERROR's variant: DRIVE 50, D+ TYPE RECT, SUB 766 ms) */
#define KICK_PRESETS_MEM 6u              /* (a kick memory whose PRESET was never stored (kick_lab_t.pre 0): a save from
                                          * before TERROR, its memories begun as the first six, round; since, every memory
                                          * begins as the eight in order, its PRESET stored: kmem_extra) */
extern const char *const KICK_PRESET_NAME[KICK_PRESETS];
/* PRESET's round (K1, OUT K4) and the kick memories' first eleven, by table index: 909, 909 DRIVE, TECHNO, EARLY HC,
 * ROTTERDAM, INDUSTRIAL, TERROR, SPINE, GABBER, MILLENNIUM, DOOMCORE (0.9.39, FLEXOR) */
extern const uint8_t KICK_PRESET_ORD[KICK_PRESETS_ON];
void kick_preset(kick_t *k, uint32_t i);   /* the sound of a built-in kick (the volume kept) */
void kick_defaults(kick_t *k);           /* ROTTERDAM, volume 0.8 */
float kick_pitch_hz(uint32_t layer, float pitch);   /* a layer's pitch knob (0..1) in Hz, for the screen */
float kick_decay_ms(uint32_t layer, float decay);   /* its decay knob in ms (to -40 dB) */
/* the effects' settings as the screen shows them: FILTER's cutoff and RING's frequency in Hz, CRUNCH's bits */
float kick_fx_hz(uint32_t fx, float a);
uint32_t kick_fx_bits(float a);

/* LOAF BREAKER: the KICK LAB, each kick's own (all 0: as it always sounded) */
typedef struct {
    uint8_t mode;                        /* TAIL MODE: 0 CLASSIC (as it always was), 1 CLEAN: the distortion before the
                                          * tail's fall (its timbre kept as it fades, the loudness as the clip made it),
                                          * PUNCH and GRIT on the SUB's octave, GRIT's FM harmonic, the RUMBLE's loop a
                                          * whole number of the note's periods; 2 SPLIT (0.9.71, FLEXOR): the body as
                                          * CLASSIC, then the tail a clean sine on the SUB's note (from `amt`); 3 DIRTY:
                                          * the drive low on the hit, rising along the tail (over `amt`) */
    uint8_t note;                        /* NOTE: 0 FREE (the SUB's PITCH), 1..24 the SUB on A#0 .. A2 (MIDI 22 + n) */
    uint8_t curve;                       /* the pitch drop's shape: 0 EXP (as always), 1 LINEAR, 2 SNAP (up, then down
                                          * fast); the last two land on the note at half the drop's TIME, and stay */
    uint8_t end;                         /* END: 0 off, 1..49: the kick cut at 20 + 20 n ms (40 .. 1000 ms; 5 ms fade);
                                          * 50..KICK_END_MAX: 1000 + 50 (n - 49) ms, to 3 s (0.9.70) */
    uint8_t filt;                        /* TAIL FILTER: 0 off, 1..100: the kick low-passed, from open down to ~11 kHz ..
                                          * 190 Hz as it goes (two poles) */
    uint8_t ftime;                       /* how fast it closes, 0..100: 20 .. 600 ms */
    uint8_t drive;                       /* TAIL DRIVE 0..100: after the attack, the kick pushed up to 9x harder into
                                          * the clip */
    uint8_t src;                         /* STACK: 0 off, 1..10: the attack from KICK n, this kick the tail */
    uint8_t cross;                       /* where the attack hands over, 0..100: 10 .. 300 ms */
    uint8_t fade;                        /* the hand-over's length, 0..100: 2 .. 150 ms */
    uint8_t pre;                         /* (the app's, not the engine's: the preset this kick came from, 1 + OUT's
                                          * K4 PRESET's place; 0: the kick memory's own built-in, 0.9.2) */
    uint8_t amt;                         /* (0.9.71, the byte left free before) TAIL MODE's own setting, 0..100: SPLIT
                                          * where the tail turns clean (KLM_SPLIT), DIRTY how long the dirt takes to
                                          * rise (KLM_RISE); CLASSIC, CLEAN: none */
} kick_lab_t;                            /* 12 bytes */
enum { KTM_CLASSIC, KTM_CLEAN, KTM_SPLIT, KTM_DIRTY, KTM_N };
#define KICK_END_MAX 89u                 /* (0.9.70, FLEXOR: END to 3 s, "1000 ms ne suffit pas") */
float kick_lab_ms(uint32_t what, uint32_t v);   /* KL_END, KL_CROSS, KL_FADE, KL_FTIME as ms; KL_FILT, KL_NOTE as Hz */
enum { KLM_END, KLM_CROSS, KLM_FADE, KLM_FTIME, KLM_FILT, KLM_NOTE, KLM_SPLIT, KLM_RISE };
void kick_set_lab(const kick_lab_t *l);  /* main loop: the kick playing's */
/* LOAF BREAKER (0.9.45, FLEXOR's Kick Lab 2: chosen by ear, HARD 909's dry attack and ROLL's chopped tail): TOK, a short
 * pitched attack in front of the layers, into the clip with them (the dirt takes it too); the TAIL GATE, the kick
 * chopped in time from its second 16th on. Each kick its own (the app keeps them apart from kick_lab_t); all 0: as the
 * kick always sounded */
typedef struct {
    uint8_t tok;                         /* TOK: 0 off, 1..100 its level */
    uint8_t tok_type;                    /* 0 SQUARE (the hard tok), 1 NOISE (a burst in a band), 2 SINE (rounder) */
    uint8_t tok_pitch;                   /* 0..100: where it starts, 400 .. 4000 Hz (it falls to a third in ~4 ms); NOISE
                                          * its band */
    uint8_t tok_len;                     /* 0..100: 2 .. 30 ms */
    uint8_t gate;                        /* TAIL GATE: 0 off, 1 1/8, 2 1/12, 3 1/16, 4 1/24, 5 1/32 */
    uint8_t gate_len;                    /* the part of each chop that sounds, 0..100: 10 .. 90 % */
    uint8_t gate_fade;                   /* 0: chopped at once; 1..100: the chop coming in over 20 .. 1000 ms */
    uint8_t rsv;
} kick_x2_t;
#define KICK_TOK_TYPES 3u
#define KICK_GATES 6u
void kick_set_x2(const kick_x2_t *x);    /* main loop: the kick playing's */
/* main loop: STACK's attack, its kick, SYNTH and LAB (its own STACK not heard); k 0: off */
void kick_set_stack(const kick_t *k, const int8_t *syn, const kick_lab_t *l);
void kick_set_solo(uint32_t s);          /* STACK's SOLO: 0 both, 1 the attack alone, 2 the tail alone */

void kick_init(void);
void kick_set(const kick_t *k);          /* main loop: read at the next render */
/* LOAF BREAKER: the kick's SYNTH settings (EDIT KICK's SYNTH and SYNTH +), -50 .. +50 each (0: as it always sounded):
 * KS_DROP the SUB's pitch drop (how far above it starts: x0.25 .. x4 of its octave and a half), KS_TIME how long the
 * drop takes (30 .. 480 ms), KS_SHAPE the SUB's wave (a clean sine .. squared off), KS_HOLD the SUB held at full before
 * it decays (to 250 ms; below 0: a faster decay), KS_RUMBLE a rumble under it (the kick low-passed into a feedback
 * delay: the hardcore rumble; 0 or below: none), KS_RTIME the rumble's length, KS_ASYM an uneven drive (even
 * harmonics, warmer and harder; 0 or below: none), KS_TUNE the whole kick, in semitones (-12 .. +12) */
enum { KS_DROP, KS_TIME, KS_SHAPE, KS_HOLD, KS_RUMBLE, KS_RTIME, KS_ASYM, KS_TUNE, KS_N };
void kick_set_synth(const int8_t *s);    /* main loop, KS_N of them */
/* LOAF BREAKER: a hit of k drawn as its wave (the screen's KICK menu): n columns over ms, each the column's sample
 * furthest from 0 (signed, -height .. +height) */
void kick_wave(const kick_t *k, int8_t *px, uint32_t n, uint32_t ms, uint32_t height);
void kick_rumble_buf(int16_t *buf, uint32_t len);   /* boot: the rumble's line (the app's: in the pool) */
void kick_hit(void);                     /* main loop (a MIDI note): a kick at the next render */
/* LOAF BREAKER TEST: GLIDE, the kick's pitch sliding from the last hit's note to the next's over glide_ms (0: a jump);
 * the REVERSE BASS, the kick's sub at its note swelling up between two kicks (rawstyle): its level 0..1, where in the
 * beat it starts (0..1: 1/8 .. 5/8 of a beat), its curve (0 slow .. 1 sharp, at the end); it goes through the kick's
 * drive and TERROR. Main loop */
void kick_set_bass(float glide_ms, float rb_level, float rb_start, float rb_curve);
void kick_set_beat(float spb);           /* render, each block: samples a beat (the REVERSE BASS's beat) */
/* LOAF BREAKER: CLEAN SUB, 0..1: a pure sine at the SUB's note (its drop, its envelope) laid after all the
 * distortion, the distorted kick's own lows (under ~90 Hz) taken out as much: round bass under the dirt. Main loop */
void kick_set_clean_sub(float amount);
/* render: n samples into out (overwritten); hits at the sample offsets at[0..nat-1] (rising) */
void kick_render(float *out, uint32_t n, const uint16_t *at, uint32_t nat);
/* LOAF BREAKER: the next render's speed, a sample each (0..1: the VINYL BREAK slowing the kick like a record), or 0
 * (as it is). Set from the render, before kick_render */
void kick_speed(const float *sp);
/* LOAF BREAKER (crossbreed): the next render's hits' pitches, a factor each (pm[k] for at[k]; 0: as set). Set from the
 * render, before kick_render */
void kick_pitches(const float *pm);
/* LOAF BREAKER: a hit of k in rekordbox's RGB (the screen's KICK menu, RGB view): n columns over ms, each its lows
 * (under ~200 Hz), mids and highs (over ~2.5 kHz) and its peak, 0..255 each (the CDJ view's bands and weights) */
void kick_rgb(const kick_t *k, uint8_t (*col)[4], uint32_t n, uint32_t ms);
/* the kick's shape, for the screen: n peaks (0..1) of the first ms milliseconds, rendered offline (main loop) */
void kick_shape(const kick_t *k, uint8_t *px, uint32_t n, uint32_t ms, uint32_t height);
