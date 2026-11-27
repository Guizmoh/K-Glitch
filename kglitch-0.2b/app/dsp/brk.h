/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's break engine: a loop cut into slices, played in time by a 16th-note sequencer, rearranged by a
 * generator and mangled by performance pads. No platform code: it builds with plain gcc (tests/engine) as well as
 * for the FM-1 (an -O2 unit, fm1.toml o2) and the browser.
 *
 * Threads: the brk_* setters are called from the main loop (the UI), brk_render from the audio interrupt. Key
 * presses go through a lock-free single-producer queue; parameters are single words written whole. */
#pragma once
#include <stdint.h>

#define BRK_SR 44100u                    /* the render's rate */
#define BRK_STEPS_PER_BAR 16u
#define BRK_MAX_SLICES 32u
#define BRK_MAX_STEPS 64u                /* a pattern: up to four bars of 16ths */
#define BRK_NO_SLICE (-1)

/* the black keys, left to right (PRD 5.1) */
enum { PAD_ROLL8, PAD_ROLL16, PAD_ROLL32, PAD_REVERSE, PAD_FREEZE, PAD_HALF, PAD_TAPESTOP, PAD_GATE, PAD_RANDOM,
       PAD_FILTER, PAD_DUB, BRK_NPADS };

/* a break in RAM: mono int16 PCM at its own rate, its tempo, and where its slices start (sample offsets, rising,
 * slice[nslices] = len) */
typedef struct {
    const int16_t *pcm;
    uint32_t len;
    uint32_t rate;                       /* Hz */
    uint32_t bpm_x100;                   /* the tempo it was played at, 13690 = 136.90 BPM */
    uint32_t bars;                       /* whole bars in the loop (1..4) */
    uint32_t nslices;                    /* 1..BRK_MAX_SLICES */
    uint32_t slice[BRK_MAX_SLICES + 1];
    uint32_t split;                      /* a split break (PRD S-030): slices below it are its first half (bar 1,
                                          * break A), the rest its second (bar 2, break B); 0 none */
    char name[13];
    uint8_t roles[BRK_MAX_SLICES];       /* (LOAF BREAKER 0.9.75, from CHOPPAH's MATHCORE Slice Map) each slice's role
                                          * bits, 1 << BRK_ROLE_*, heard from its audio (brk_auto_roles); 0 none */
} brk_break_t;
enum { BRK_ROLE_KICK, BRK_ROLE_SNARE, BRK_ROLE_HAT, BRK_ROLE_PERC };

/* one step of a pattern (PRD B-21); slice BRK_NO_SLICE follows the loop */
typedef struct {
    int8_t slice;
    uint8_t on;
    int8_t pitch;                        /* semitones */
    uint8_t flags;                       /* STEP_REVERSE ... */
    uint8_t ratchet;                     /* 1..4 */
    uint8_t prob;                        /* 0..100 */
    uint8_t rsv[2];
} brk_step_t;
#define STEP_REVERSE 1u

typedef struct {
    float complexity, anchor, roll, fill;    /* 0..1 (PRD A-10) */
    uint32_t phrase_bars;                     /* 0 off, 2, 4, 8, 16 */
} brk_gen_t;

/* the pads' own settings, tuned with K1-K4 while a pad is held (PRD 5.1) */
#define BRK_ROLL_DIVS 11u                /* 1/4 1/8 1/16 1/32 1/64 1/8T 1/16T 1/32T, then 3/8 3/16 3/32 (0.9.30, FLEXOR: the
                                          * keys' dotted lengths; at the end so saved rates keep theirs; RATE turns through
                                          * them in BRK_ROLL_ORDER) */
#define BRK_GATE_DIVS 3u                 /* 1/8 1/16 1/32 */
typedef struct {
    uint8_t roll_div[3];                 /* each roll pad's rate (0..BRK_ROLL_DIVS-1): 1/8, 1/16, 1/32 by default */
    uint8_t frz_len;                     /* freeze loop: 0 the whole slice, 1 half, 2 a quarter, 3 an eighth */
    uint8_t gate_div;                    /* 0..BRK_GATE_DIVS-1 */
    uint8_t roll_level;                  /* the roll's repeats, % of full: 5..100, ghost notes low (0, in older saves:
                                          * 100) */
    uint8_t rsv[2];
    float roll_decay;                    /* -0.9..0.9: > 0 each repeat this much quieter, < 0 louder (a swell up to
                                          * full) */
    float roll_pitch;                    /* -12..+12 semitones a repeat (the sum held to +-36) */
    float rev_speed;                     /* 0.5..2 */
    float frz_pitch;                     /* -12..+12 semitones */
    float half_speed;                    /* 0.25..0.75 */
    float stop_beats;                    /* tape stop: 0.25..16 beats down (LOAF BREAKER: 4 bars) */
    float spin_steps;                    /* and 0.25..8 16ths back up */
    float gate_len;                      /* 0.1..0.9 of each gate */
    float rnd_amount;                    /* 0..1: the chance a step plays a random slice */
    float rnd_pitch;                     /* 0..12 semitones of random pitch */
} brk_padset_t;
extern const char *const BRK_ROLL_DIV_NAME[BRK_ROLL_DIVS];
extern const uint8_t BRK_ROLL_ORDER[BRK_ROLL_DIVS];   /* 3/8 1/4 3/16 1/8 3/32 1/16 1/32 1/64, the triplets */

/* the white keys' own settings, tuned with K1-K4 while a key is held (PRD B-10a). Held, a key loops a window that
 * starts in its slice: LENGTH slices long (1/8 .. 16 of them), from START sixteenths into the slice, at PITCH, in
 * MODE. The loop restarts every LENGTH on the tempo's grid, playing or stopped. */
#define BRK_KEYS 16u
#define BRK_KEY_LENS 11u                 /* 1/8 1/4 1/2 1 2 4 8 16 slices, then 1.5, 3 and 0.75 (3/16, 3/8, 3/32 (0.9.30) of the bank's
                                          * 2-bar breaks, 16 slices each: the jungle chops; CHOPPAH 1.3.1's, 0.9.29: at the
                                          * end so saved lengths keep theirs; LENGTH turns through them in order, BRK_KEY_ORDER) */
enum { KEY_LOOP, KEY_PINGPONG, KEY_REVERSE, KEY_ONESHOT, BRK_KEY_MODES };
typedef struct {
    uint8_t len;                         /* 0..BRK_KEY_LENS-1: 3 = one slice */
    int8_t pitch;                        /* -12..+12 semitones */
    uint8_t start;                       /* 0.9.50, FLEXOR: bits 4-7 DECAY (0 none, 1..15: each repeat fades, BRK_KEY_DECAY_MS;
                                          * START gone: bits 0-3, an older save's sixteenths in, read as 0) */
    uint8_t mode;                        /* KEY_* */
} brk_keyset_t;                          /* 4 bytes: written to the render as one word */
extern const float BRK_KEY_SLICES[BRK_KEY_LENS];
float brk_key_decay_ms(uint32_t d);     /* (0.9.50: DECAY d, 1..15: each repeat to -40 dB in 25 .. 1970 ms; 0: none) */
extern const uint8_t BRK_KEY_ORDER[BRK_KEY_LENS];   /* the lengths shortest first: 0 1 2 10 3 8 4 9 5 6 7 */

/* each slice's own effect (PRD B-32), whenever that slice plays (the loop, a key, a roll): a filter, crunch, drive or
 * ring mod on the slice alone, or a throw of it into the FX chain's delay (ECHO) or reverb (VERB); and its own level.
 * amount, tone and level are 0..100 */
enum { SLX_OFF, SLX_LOPASS, SLX_HIPASS, SLX_CRUNCH, SLX_DRIVE, SLX_RING, SLX_ECHO, SLX_VERB, BRK_SLX };
extern const char *const BRK_SLX_NAME[BRK_SLX];
typedef struct {
    uint8_t type;                        /* SLX_* */
    uint8_t amount;                      /* LO-PASS: darker; HI-PASS: thinner; CRUNCH, DRIVE: more; RING: its pitch;
                                          * ECHO, VERB: the send */
    uint8_t tone;                        /* the filters' resonance; RING's mix */
    uint8_t level;                       /* the slice's level */
} brk_slicefx_t;                         /* 4 bytes: a word to the render */
extern const char *const BRK_KEY_MODE_NAME[BRK_KEY_MODES];
extern const char *const BRK_GATE_DIV_NAME[BRK_GATE_DIVS];

/* what the UI shows (a copy, read any time) */
typedef struct {
    int32_t step;                        /* in the pattern, -1 stopped before the first */
    int32_t slice;                       /* sounding, -1 none */
    uint32_t bar;
    float pos;                           /* the playhead in the break's samples */
    uint32_t pads;                       /* bit PAD_*: active */
    uint8_t playing;
    uint8_t roll_div;                    /* the roll's rate (BRK_ROLL_DIVS), while one is held */
    uint16_t roll_n;                     /* repeats since the roll began */
    int32_t roll_slice;                  /* the slice the roll repeats, -1 none */
    float tape;                          /* tape speed: 1 running .. 0 stopped */
    float gate;                          /* the gate's level now, 0..1 */
    float phase;                         /* how far into the step, 0..1 */
    int32_t key;                         /* the white key looping (its slice), -1 none */
    float key_lo, key_w;                 /* its window: from key_lo, key_w samples of the break (it may run on round
                                          * the break's end) */
    float key_phase;                     /* how far into this repeat, 0..1 */
    uint16_t key_n;                      /* repeats since the key went down */
    uint8_t key_rev;                     /* this repeat plays backwards */
    int8_t bar16;                        /* the step's place in the bar, 0..15, -1 none yet */
    int8_t bar64;                        /* its place in four bars, 0..63 (as brk_step_events), -1 none yet */
    int8_t half;                         /* the split's half in force (brk_half): -1 both, 0 the first, 1 the second */
    int32_t layer;                       /* the layer's key looping (its slice), -1 none (PRD S-032) */
    float layer_lo, layer_w, layer_phase;   /* its window and how far into this repeat, as the key's */
    float layer_pos;                     /* its playhead in the break's samples, -1 quiet */
} brk_status_t;

void brk_init(void);
void brk_set_break(const brk_break_t *b);   /* main loop; takes effect at once (call stopped, or at a bar) */
void brk_mute(int on);                   /* the break silent (an 8 ms fade), the clock and the steps running on */
void brk_swap_break(const brk_break_t *b);   /* b takes over at the next step (stopped: the next render), its
                                          * voices clean, and the mute ends. Swapping while the old break
                                          * plays muted lets the main loop rewrite its PCM meanwhile */
const brk_break_t *brk_get_break(void);

void brk_play(int on);
void brk_reset(void);                    /* to the pattern's first step (the next step played is step 0) */
/* LOAF BREAKER: the next step (at its time: no jump) is the pattern's first, step 0, and the drums' first bar: a new
 * pattern starts at its step 1, not where the old one was */
void brk_rewind(void);
void brk_tempo(float bpm);               /* 40..300 */
void brk_half_time(int on);             /* LOAF BREAKER: 1, the break at half the tempo (its steps every other 16th,
                                          * its own speed); the drums, the clock at the tempo */
float brk_get_tempo(void);
void brk_pitch(float semitones);         /* -24..+24, on top of the tempo's repitch */
/* how pitch sounds (PRD E-9): SPEED, the classic repitch (up is shorter, down longer), or KEEP LENGTH: the voices
 * run at the tempo's speed and two overlapping grains of grain_ms (10..80) read them at the pitch, so a slice keeps
 * its length. Applies to every pitch in semitones (OCT, steps, a key's, the roll's and the freeze's); the tempo's
 * repitch, HALF, REVERSE's speed and the tape stop stay speed */
void brk_pitch_mode(int keep_length, float grain_ms);
void brk_half_master(int on);            /* GLOUPI BREAKS: HALF SPEED keeps the pitch (1, the start) or drops it */
/* LOAF BREAKER: the VINYL BREAK pad (PAD_TAPESTOP) slows the break (1, the start), or not (0: the drums alone, the
 * app's kick_speed / perc_speed; the status's tape still runs) */
void brk_vinyl(int on);
/* a split break (brk_break_t.split, PRD S-030 to S-032): a white key held in the half the key looping isn't in layers
 * on voices of its own (one key a half; within a half the last pressed wins). brk_half keeps the loop to one half
 * (0, 1; -1 both, the default): the change waits for the next bar line, and the loop's own slices (the pattern's,
 * the generator's, RANDOM's) fold into that half; keys still play any slice. Without a split it does nothing */
void brk_half(int h);
int brk_get_half(void);                  /* the half asked for */
void brk_swing(float s);                 /* 0.5 straight .. 0.75 */
/* the gate (PRD B-30): how much of each slice the loop plays sounds, 0.05..1 (1: all of it). A slice gated at g
 * fades out once it has played g of its own length (at whatever speed), so it follows the tempo and the pitch. For
 * the loop's slices, ratchets and the RANDOM pad; key loops and rolls have their own lengths */
void brk_gate(float g);
/* the snap (PRD B-31): where a white key pressed while playing begins, in slices of the break: 0 at once, else at the
 * next point of a grid that many slices apart from the pattern's start (1/8 .. 4; 0.5 is the next 16th on a 2-bar
 * break of 16 slices, the default). Whole steps follow the swing; less than a step is timed straight */
void brk_snap(float slices);

/* the white keys: a press plays slice s at the next snap point (brk_snap; at once when stopped: free play); held, it
 * loops its window (brk_keyset) until released, the last key pressed winning */
void brk_key(int slice, int down);
void brk_keyset_defaults(brk_keyset_t *k);   /* one slice, no pitch, from the slice's start, LOOP */
void brk_keyset(uint32_t key, const brk_keyset_t *k);   /* main loop; the next repeat follows it */
void brk_slicefx_defaults(brk_slicefx_t *f);   /* off, level 100 */
void brk_slicefx(uint32_t slice, const brk_slicefx_t *f);   /* main loop; the slice's next hit follows it */
/* the render, before brk_render: where it writes the slices' throws (ECHO, VERB) for the effects; 0: none */
void brk_sends(float *echo, float *verb);
void brk_pad(int pad, int down);
void brk_gen(const brk_gen_t *g);
/* LOAF BREAKER (0.9.75, FLEXOR: "une fonction un peu mathcore lite ... scramble ... pour rajouter du chaos", the break):
 * HOME › CHAOS. b's roles heard (main loop, when a break is loaded: CHOPPAH's MATHCORE Slice Map, MPL-2.0); then each
 * step the break plays (the loop, the generator, the pattern's slices) goes through CHAOS: packed as amount 0..100
 * (bits 0-7: a hit swapped for another of the same role, kick for kick, snare for snare; reversed above 50), MATH
 * (bits 8-15: 0 off, else N, each bar's first N 16ths round and round against the bar: 3, 5, 7, 11, 13), ROLLS
 * (bits 16-23, 0..100: snares rolled in three, hats in two) and SEED (bits 24-31: which variation; the same seed, the
 * same chaos every loop, so it grooves). 0: the break as it is */
void brk_auto_roles(brk_break_t *b);
void brk_chaos(uint32_t packed);
/* K-GLITCH 0.2b: the CHOP effects, K-GLITCH's rhythmic destroyers on the break's steps (no audio buffer: each one rewrites
 * the step the sequencer plays: its slice, direction, pitch, ratchet, or no hit). live: bits 0-4 the effect held (0
 * none), bits 8-15 its amount 0..100; lane: the CHOP lane, a byte a step (bits 0-4 the effect, bits 5-7 its amount in
 * eighths, 0 = the live amount), len its steps (0: off). A held effect wins over the lane's */
enum { KGC_NONE, KGC_STUTTER, KGC_REPEAT8, KGC_REPEAT4, KGC_REVERSE, KGC_SKIP, KGC_HALF, KGC_DOUBLE, KGC_PITCH,
       KGC_SCRATCH, KGC_BACKSPIN, KGC_SHUFFLE, KGC_GATE, KGC_MICRO, KGC_TRIPLET, KGC_TAPESTOP, KGC_CHAOS,
       /* the DESTROY bank (17..32), K-GLITCH's DRUM DESTROYER on the steps (RING MOD, FUZZ and FILTER CHAOS through the
        * ARP effects: the app, brk_kg_ovl) */
       KGD_LOOP, KGD_STUTX, KGD_GLITCH, KGD_SCRATCH, KGD_REVX, KGD_PITCH, KGD_GATEX, KGD_FILTER, KGD_BREP, KGD_BSKIP,
       KGD_MICRO, KGD_STRETCH, KGD_RING, KGD_FUZZ, KGD_RAND, KGD_TOTAL,
       /* the BEAT bank (33..48): programs over the bar, each 16th its own operation */
       KGB_RSTUT, KGB_RBAR, KGB_RFILL, KGB_ROLL, KGB_SKIP8, KGB_STAMR, KGB_GATE8, KGB_GATE16, KGB_GATE332, KGB_GRAMP,
       KGB_REVB, KGB_REVOFF, KGB_SCRBAR, KGB_SCRCUT, KGB_TSTOP, KGB_HALFT, KGC_N };
#define KGC_BANKS 3u                     /* CHOP, DESTROY, BEAT: 16 effects each, the white keys */
extern const char *const KGC_BANK_NAME[KGC_BANKS];
extern const char *const KGC_NAME[KGC_N];
extern const char *const KGC_SHORT[KGC_N];
void brk_kg(uint32_t live, const uint8_t *lane, uint32_t len);   /* (0.2b: lane bytes bits 0-5 the effect, 6-7 its
                                          * amount in thirds, 0 = the live amount; live bits 0-5) */
void brk_kg_cond(const uint8_t *cnd, uint32_t density);   /* (K-GLITCH 0.2b: each lane step's trigger, a byte: bits 0-3
                                          * the condition KGQ_*, 4-7 its chance less: 0 100 %, n 100 - n * 6.25 %; density
                                          * 0..100 % scales every chance) */
enum { KGQ_ALWAYS, KGQ_1_2, KGQ_2_2, KGQ_1_3, KGQ_2_3, KGQ_3_3, KGQ_1_4, KGQ_2_4, KGQ_3_4, KGQ_4_4, KGQ_FIRST, KGQ_NFIRST,
       KGQ_PREV, KGQ_NPREV, KGQ_N };
int32_t brk_kg_now(void);                /* the effect playing on this step (live or the lane's), 0 none */
void brk_padset_defaults(brk_padset_t *p);
void brk_padset(const brk_padset_t *p);  /* main loop; the render follows it at once, also while a pad is held */
void brk_seed(uint32_t seed);

/* the pattern: the break's own length in steps, all "follow, on" by default (the app's sequencer writes it) */
void brk_step_set(uint32_t i, const brk_step_t *s);
void brk_step_get(uint32_t i, brk_step_t *s);
void brk_randomize(void);                /* writes random slices into the pattern (PRD A-11) */

/* MIDI clock (PRD N-1, N-2). Out: while playing, the render marks MIDI's 24 ticks a beat (straight, whatever the
 * swing; the first on step 0): brk_clock_ticks, called after brk_render in the same context, gives the sample offsets
 * of the last render's. In: brk_sync, called from the render before brk_render, says the outside clock's 16th n16
 * (counted from its start) began age samples ago; the loop eases its steps onto it (or jumps, when far off). The
 * tempo itself comes from brk_tempo. */
uint32_t brk_clock_ticks(uint16_t *off, uint32_t max);
void brk_sync(uint32_t n16, float age);
/* the steps begun in the last render (called after brk_render, in its context): their sample offsets, and each one's
 * place in four bars (0..63: & 15 its place in the bar; counted from the reset like the outside clock's, whatever the
 * loop's length). The drums' lanes play from these (the kick's over four bars). */
uint32_t brk_step_events(uint16_t *off, uint8_t *idx, uint32_t max);

/* audio interrupt: n mono frames into out[] (-1..1 floats). The FX and the output stage are the caller's
 * (brk_fx.h) */
void brk_render(float *out, uint32_t n);
void brk_status(brk_status_t *st);
uint32_t brk_renders(void);              /* renders so far: two more since brk_set_break(0) = the old break let go */

/* the host tests' view */
uint32_t brk_step_len_samples(void);
