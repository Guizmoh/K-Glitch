/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH: the DRUM DESTROYER bank, 16 effects on the white keys (and the BEAT bank's 16: kg_beat.c) (FX held, BANK: DESTROY), in the effect sequencer
 * and in AUTO. Included by kglitch.c; runs in the audio ISR after punch_process (fx.c mix_block: kd_process), on the
 * whole mix. Its past is SLOOP's punch ring (punch.c: the dry mix, mono, 0.74 s): a held segment is kept from the
 * ring's writer (punch.c asks kd_owns). INTENSITY (FX > BANK) shapes every effect:
 *   LOOP DESTROY  the bar cut into 1/2 .. 1/32 (INTENSITY), looped; past 60 % each pass forward or backward, past
 *                 85 % from a random quarter of the fragment
 *   STUTTER X     2x, 3x, 4x, 8x a beat (INTENSITY), each repeat fading; past 75 % random bursts (1/2, 1/4 the length)
 *   GLITCH        bit crush and sample-rate reduction, buffer corruption (a chunk of the past instead of now) and, past
 *                 60 %, digital noise bursts
 *   SCRATCH       a hand on the record (kg_beat.c): strokes of the last attack heard, each 16th, under a crossfader;
 *                 INTENSITY: baby scratches, chirps, then tears, transformers, scribbles, drags, backward cuts
 *   REVERSE X     the last beat, 1/2 or 1/4 (INTENSITY) played backwards, segment after segment; past 70 % some
 *                 segments forwards (delayed)
 *   PITCH MAD     a pitch shift that moves each 16th: up a staircase (low), random jumps (middle), a dive (high)
 *   GATE X        a pattern of silences each bar, 16ths or 32nds, more holes with INTENSITY
 *   FILTER CHAOS  a resonant low-pass jumping to a random cutoff each 16th, resonance and range with INTENSITY
 *   BEAT REPEAT   the last beat, 1/2 or 1/4 repeated, fading; past 70 % each repeat half as long (a roll)
 *   BEAT SKIP     16ths dropped, or past 50 % replaced by the one before
 *   MICRO STUTTER 50 .. 10 ms grains of the last instant, a new one each 16th
 *   TIME STRETCH  the groove slowed (3/4 .. 1/4 speed) without changing its pitch (two overlapping grains)
 *   RING MOD      a sine carrier, 30 Hz .. 2 kHz
 *   FUZZ          drive (x1 .. x16) into a soft then hard clip, its level kept
 *   RANDOMIZER    one of the 14 above, a new one each 16th
 *   TOTAL CHAOS   the same, crushed, with a ring modulator on some 16ths
 * Every switch fades over 64 samples (no click); the output stays bounded (ring reads are the stored mix, the gains
 * at most 1, the fuzz clipped). */

enum { DD_LOOP, DD_STUT, DD_GLITCH, DD_SCRATCH, DD_REV, DD_PITCH, DD_GATE, DD_FILTER,
       DD_BREP, DD_BSKIP, DD_MICRO, DD_STRETCH, DD_RING, DD_FUZZ, DD_RAND, DD_TOTAL,
       /* the BEAT bank (kg_beat.c): programs over the bar */
       DD_RSTUT, DD_RBAR, DD_RFILL, DD_ROLL, DD_SKIP8, DD_STAMR, DD_GATE8, DD_GATE16, DD_GATE332, DD_GRAMP,
       DD_REVB, DD_REVOFF, DD_SCRBAR, DD_SCRCUT, DD_TSTOP, DD_HALFT, DD_N };
#define DD_BANK 16u                               /* effects a bank (DESTROY: 0..15, BEAT: 16..31) */
#define KG_NFX (PUNCH_NFX + DD_N)                 /* the effect ids K-GLITCH knows: PX_*, then PUNCH_NFX + DD_* */
static const char *const DD_NAME[DD_N] = {"LOOP DESTROY", "STUTTER X", "GLITCH", "SCRATCH", "REVERSE X", "PITCH MAD",
                                          "GATE X", "FILTER CHAOS", "BEAT REPEAT", "BEAT SKIP", "MICRO STUT",
                                          "TIME STRETCH", "RING MOD", "FUZZ", "RANDOMIZER", "TOTAL CHAOS",
                                          "RND STUTTER", "RND BAR", "RND FILL", "ROLL UP", "SKIP 8TH", "STAMMER",
                                          "GATE 8", "GATE 16", "GATE 3-3-2", "GATE RAMP", "REV BEAT", "REV OFFBEAT",
                                          "SCRATCH BAR", "SCRATCH CUT", "TAPE STOP", "HALF TIME"};
#define DD_W 2048u                                /* PITCH and STRETCH: the grain window (46 ms) */

typedef struct {
    int8_t cur, sub;          /* the effect running (DD_*, -1 none); what plays this 16th (RANDOMIZER, TOTAL: picked) */
    int32_t g;                /* its wet gain, 0..32767 */
    int32_t xf;               /* a sub-effect's fade in, 0..64 */
    uint32_t t;               /* samples since it started */
    uint32_t s16;             /* the 16th last seen */
    uint32_t S, L, L0, k;     /* a segment of the ring: start, length (L0: as captured), position */
    uint8_t rev, own, mode;   /* this pass backwards; the segment is held from the writer; SKIP / TOTAL: this 16th's way */
    uint32_t off;             /* LOOP: offset into the fragment */
    int32_t rep;              /* STUT, BREP: this repeat's level, Q15 */
    uint32_t ri;              /* SCRATCH, STRETCH: a read position (ring index) and its fraction (Q16) */
    int32_t rf, sp;           /* .. and the speed, Q16 */
    uint32_t ph, dph, ph2;    /* an oscillator: SCRATCH's push, RING's carrier; TOTAL's ring */
    uint32_t pp;              /* PITCH: the delay phase, Q16 over DD_W; STRETCH: the grains' phase */
    int32_t ratio;            /* PITCH: Q16; STRETCH: the speed, Q16 */
    uint32_t ga, gb;          /* STRETCH: the two grains' anchors */
    int8_t semi;              /* PITCH: the step it is on */
    uint32_t mask, i32;       /* GATE: the bar's pattern (bit per 16th or 32nd), the slot last seen */
    int32_t gg;               /* GATE / SKIP: the gain, Q15 */
    int32_t f1l, f2l, f1r, f2r, cut, cut_to, reso;   /* FILTER */
    int32_t hl, hr;           /* GLITCH: the held sample */
    uint32_t hn, corrupt, csrc, noise;   /* GLITCH: hold count, corrupted samples left, their source, noise left */
    uint32_t rnd;
    /* BEAT and SCRATCH (kg_beat.c): the bar's seed, the record, the 16th's length and the samples into it, the stroke's
     * depth, its progress (Q24) and step */
    uint32_t bseed, rec, slen, sk, D, u24, du;
    int32_t acc, lpy, ps, dl, dr, ll, lr;   /* STOP's slowdown; the cartridge's low-pass; the record's last place (Q15);
                                             * the jump's declick and the last output */
    uint8_t op, arg, open, newop, qi, from8, to8;   /* the 16th's operation (BO_*), open (gate, fader, mute) */
} kd_t;
#define KD_SLOTS 3u                               /* MULTI: three DRUM DESTROYER effects at once, one after the other */
static kd_t kd_slot[KD_SLOTS] = {{.cur = -1, .sub = -1, .rnd = 0x51A3E1B7u}, {.cur = -1, .sub = -1, .rnd = 0x2F6B1D3Bu},
                                 {.cur = -1, .sub = -1, .rnd = 0x7E3A5C19u}};
static kd_t *kdp = &kd_slot[0];                   /* the slot running (kd_process); the code below reads it as kd */
#define kd (*kdp)
static volatile int8_t kd_wantv[KD_SLOTS] = {-1, -1, -1};   /* the effect each slot should play (kg_route), -1 none */
static volatile uint8_t kd_sintv[KD_SLOTS];       /* its recorded INTENSITY + 1 (0: the effect's own) */

/* INTENSITY: each effect its own (FX > BANK K2, PRESETS on the K-GLITCH pages), 0..100; the LFO (FX > BANK / CHAOS)
 * moves it by up to MOVEMENT / 2 */
static uint8_t kd_int[DD_N] = {[0 ... DD_N - 1] = 50};
static volatile int8_t kd_lfo;                   /* the LFO now, -50..50 (kg_route, per block) */
static volatile uint8_t kg_sel;                  /* the effect ALGORITHM selects (and a pad pressed): DD_* */
static volatile uint8_t kg_pads;                 /* UI: the keys are pads (1: the K-GLITCH pages) or SEQ's steps (2) */
static volatile uint8_t kg_keystep;              /* ISR: a white key pressed on SEQ, + 1 (the UI takes it) */
static volatile uint8_t kg_keynote;              /* ISR: a key pressed on FILTER: its note + 1 (the UI takes it) */
static uint32_t dd_int(void)
{
    uint32_t si = kd_sintv[(uint32_t)(kdp - kd_slot) % KD_SLOTS];
    int32_t v = (si ? (int32_t)si - 1 : kd_int[(uint32_t)(kd.cur >= 0 ? kd.cur : 0) % DD_N]) + kd_lfo;
    return (uint32_t)(v < 0 ? 0 : v > 100 ? 100 : v);
}
static uint32_t dd_beat(void) { return beat_samples(); }
/* samples into the bar (the transport's when it runs, else since the effect started) */
static uint32_t dd_barpos(void)
{
    return song.playing ? (clk_beat & 3u) * dd_beat() + clk_samples() : kd.t;
}
static uint32_t dd_cap(uint32_t L) { return L < 64u ? 64u : L > PUNCH_N / 2u - 1024u ? PUNCH_N / 2u - 1024u : L; }
static int32_t dd_rd(uint32_t i, int32_t f)        /* the ring at i + f / 65536, linear */
{
    int32_t a = ring_raw(i), b = ring_raw(i + 1u);
    return (a + (((b - a) * (f >> 1)) >> 15)) << 3;
}
static int32_t dd_declick(int32_t y, uint32_t k, uint32_t L)
{
    return k < 64u ? (y * (int32_t)k) >> 6 : L - k < 64u ? (y * (int32_t)(L - k)) >> 6 : y;
}
/* the ring's writer leaves a held segment alone once it has been written (punch.c) */
static int kd_owns(uint32_t w)
{
    uint32_t i;
    for (i = 0; i < KD_SLOTS; i++) {
        const kd_t *q = &kd_slot[i];
        uint32_t d = w - q->S;
        if (q->cur >= 0 && q->own && d >= q->L0 && (d & (PUNCH_N - 1u)) < q->L0)
            return 1;
    }
    return 0;
}
/* 2^(s/12), Q16, s -24..24 */
static int32_t dd_semi(int32_t s)
{
    static const uint32_t R[12] = {65536, 69433, 73562, 77936, 82570, 87480, 92682, 98193, 104032, 110218, 116772, 123715};
    int32_t o = 0;
    while (s < 0) s += 12, o--;
    while (s >= 12) s -= 12, o++;
    return o >= 0 ? (int32_t)(R[s] << o) : (int32_t)(R[s] >> -o);
}
#include "kg_beat.c"                              /* the BEAT bank, SCRATCH */

/* the effect e takes over from the sample at ring index `base` (its fields set; the 16th's part in dd_tick) */
static void dd_engage(int32_t e, uint32_t base)
{
    uint32_t beat = dd_beat(), I = dd_int(), pos = dd_barpos(), L;
    kd.sub = (int8_t)e;
    kd.xf = 0;
    kd.own = 0;
    kd.rev = 0;
    kd.off = 0;
    kd.rep = 32767;
    kd.gg = 32767;
    switch (e) {
    case DD_LOOP: {
        static const uint8_t DIV[5] = {2, 4, 8, 16, 32};   /* of a bar */
        L = dd_cap(beat * 4u / DIV[I * 5u / 101u]);
        kd.k = pos % L;
        kd.S = base - kd.k;                     /* the last division: the first pass is the live sound */
        kd.L = kd.L0 = L;
        kd.own = 1;
        break;
    }
    case DD_STUT: {
        static const uint8_t DIV[4] = {2, 3, 4, 8};        /* repeats a beat */
        L = dd_cap(beat / DIV[I * 4u / 101u]);
        kd.k = pos % L;
        kd.S = base - kd.k;
        kd.L = kd.L0 = L;
        kd.own = 1;
        break;
    }
    case DD_BREP:
        L = dd_cap(beat >> (I * 3u / 101u));
        kd.k = 0;
        kd.S = base - L;                        /* what just played, again */
        kd.L = kd.L0 = L;
        kd.own = 1;
        break;
    case DD_REV:
        L = dd_cap(beat >> (I * 3u / 101u));
        kd.k = pos % L;
        kd.S = base - kd.k - L;                 /* the segment before, backwards */
        kd.L = kd.L0 = L;
        kd.rev = 1;
        break;
    case DD_PITCH:
        kd.pp = 0;
        kd.semi = 0;
        kd.ratio = 65536;
        break;
    case DD_STRETCH:
        kd.ri = base - DD_W;
        kd.rf = 0;
        kd.pp = 0;
        kd.ga = kd.ri - DD_W / 2u;
        kd.gb = kd.ri - DD_W / 2u;
        kd.ratio = I < 50u ? 49152 - (int32_t)I * 16384 / 50 : 32768 - ((int32_t)I - 50) * 16384 / 50;   /* 3/4 .. 1/4 */
        break;
    case DD_GATE:
        kd.i32 = 0xFFFFFFFFu;
        break;
    case DD_FILTER:
        kd.f1l = kd.f2l = kd.f1r = kd.f2r = 0;
        kd.cut = kd.cut_to = 100 << 8;
        kd.reso = 40 + (int32_t)I * 80 / 100;
        break;
    case DD_RING:
        kd.ph = 0;
        kd.dph = (uint32_t)(30u << (I * 6u / 100u)) * (0xFFFFFFFFu / FS);   /* 30 Hz .. ~2 kHz */
        break;
    case DD_GLITCH:
        kd.hn = 0;
        kd.corrupt = kd.noise = 0;
        break;
    default:
        if (dd_isop(e))
            beat_engage(base);
        break;
    }
}

/* each 16th: what changes on the grid (s16: the transport's 16th, or since the effect started) */
static void dd_tick(uint32_t base, uint32_t s16)
{
    uint32_t I = dd_int(), r = kg_rand(&kd.rnd), step = dd_beat() / 4u;
    if (kd.cur == DD_RAND || kd.cur == DD_TOTAL) {      /* a new effect each 16th */
        dd_engage((int32_t)(r % 14u), base);   /* (any of the first 14) */
        kd.mode = (uint8_t)(kd.cur == DD_TOTAL && (r >> 8) % 3u == 0u);   /* TOTAL: a ring modulator on top */
        r = kg_rand(&kd.rnd);
    }
    switch (kd.sub) {
    case DD_BSKIP:                                     /* drop it, repeat the one before, or play it */
        kd.mode = r % 100u < I * 6u / 10u ? (I > 50u && (r >> 8) % 2u ? 2u : 1u) : 0u;
        if (kd.mode == 2u) {
            kd.S = base - step;
            kd.k = 0;
            kd.L = kd.L0 = dd_cap(step);
        }
        break;
    case DD_FILTER:
        kd.cut_to = (int32_t)(36u + r % (40u + I * 50u / 100u)) << 8;
        break;
    case DD_PITCH:
        if (I < 34u) {                                 /* a staircase up an octave */
            static const int8_t ST[8] = {0, 2, 4, 5, 7, 9, 11, 12};
            kd.semi = ST[(kd.t / (step + 1u)) % 8u];
        } else if (I < 67u) {                          /* random jumps */
            static const int8_t J[7] = {-12, -7, -5, 0, 5, 7, 12};
            kd.semi = J[r % 7u];
        } else {                                       /* a dive: an octave up down to two down, again */
            kd.semi = (int8_t)(kd.semi <= -24 ? 12 : kd.semi - 3);
        }
        kd.ratio = dd_semi(kd.semi);
        break;
    case DD_MICRO: {
        uint32_t G = 2205u - I * 1764u / 100u;         /* 50 .. 10 ms */
        kd.S = base - G;
        kd.k = 0;
        kd.L = kd.L0 = G;
        kd.own = 1;
        break;
    }
    case DD_GLITCH:
        if (r % 100u < 20u + I / 2u) {                  /* a chunk of the past instead of now */
            kd.corrupt = 256u + (r >> 8) % (256u + I * 16u);
            kd.csrc = base - 2048u - (r >> 16) % (PUNCH_N / 2u);
        }
        if (I > 60u && (r >> 4) % 100u < I - 60u)
            kd.noise = 64u + (r >> 12) % 256u;
        break;
    default:
        break;
    }
    if (dd_isop(kd.sub))                               /* BEAT, SCRATCH: the 16th's operation */
        beat_slot(s16 % 16u, base);
}

/* the wet sample of the effect playing, for the dry x, y; idx the ring index of this sample */
static void dd_sample(int32_t x, int32_t y, uint32_t idx, int32_t *wl, int32_t *wr)
{
    uint32_t I = dd_int();
    int32_t m;
    switch (kd.sub) {
    case DD_LOOP: {
        uint32_t p = kd.off + kd.k;
        if (p >= kd.L) p -= kd.L;
        m = dd_declick(ring_at(kd.S + (kd.rev ? kd.L - 1u - p : p)), kd.k, kd.L);
        if (++kd.k >= kd.L) {
            uint32_t r = kg_rand(&kd.rnd);
            kd.k = 0;
            kd.rev = (uint8_t)(I > 60u && r % 2u);
            kd.off = I > 85u ? ((r >> 8) % 4u) * (kd.L / 4u) : 0u;
        }
        *wl = *wr = m;
        return;
    }
    case DD_STUT:
    case DD_BREP:
    case DD_MICRO:
        m = dd_declick(ring_at(kd.S + kd.k), kd.k, kd.L);
        if (kd.sub == DD_STUT)                        /* each repeat fades a little */
            m = ((m >> 2) * ((65536 - (int32_t)(kd.k * 13000u / kd.L)) >> 2)) >> 12;
        else if (kd.sub == DD_BREP)
            m = (m * kd.rep) >> 15;
        if (++kd.k >= kd.L) {
            kd.k = 0;
            if (kd.sub == DD_STUT && I > 75u) {       /* bursts */
                uint32_t r = kg_rand(&kd.rnd) % 3u;
                kd.L = dd_cap(kd.L0 >> r);
            } else if (kd.sub == DD_BREP) {
                kd.rep = kd.rep * (int32_t)(500u - I) / 500;
                if (kd.rep < 16384) kd.rep = 16384;   /* (never below half) */
                if (I > 70u && kd.L > kd.L0 / 8u)
                    kd.L = dd_cap(kd.L / 2u);
            }
        }
        *wl = *wr = m;
        return;
    case DD_REV:
        m = dd_declick(ring_at(kd.S + (kd.rev ? kd.L - 1u - kd.k : kd.k)), kd.k, kd.L);
        if (++kd.k >= kd.L) {
            kd.k = 0;
            kd.S += kd.L;                             /* the next segment (already written: one behind) */
            kd.rev = (uint8_t)(I <= 70u || kg_rand(&kd.rnd) % 3u != 0u);
        }
        *wl = *wr = m;
        return;
    case DD_PITCH: {                                   /* two taps sliding through a delay, crossfaded */
        uint32_t W16 = DD_W << 16, pa = kd.pp, pb = (kd.pp + W16 / 2u) % W16;
        int32_t a = ring_at(idx - 64u - (pa >> 16)), b = ring_at(idx - 64u - (pb >> 16));
        int32_t wa = (int32_t)((pa >> 16) < DD_W / 2u ? (pa >> 16) : DD_W - (pa >> 16));
        m = (a * wa + b * ((int32_t)DD_W / 2 - wa)) / ((int32_t)DD_W / 2);
        kd.pp = (uint32_t)(((int32_t)kd.pp + (65536 - kd.ratio)) % (int32_t)W16 + (int32_t)W16) % W16;
        *wl = *wr = m;
        return;
    }
    case DD_STRETCH: {                                 /* two grains at normal speed from a point that moves slower */
        uint32_t q = kd.pp, qb = (q + DD_W / 2u) % DD_W, back;
        int32_t wa = (int32_t)(q < DD_W / 2u ? q : DD_W - q), f;
        if (q == 0u) kd.ga = kd.ri - DD_W / 2u;
        if (qb == 0u) kd.gb = kd.ri - DD_W / 2u;
        m = (ring_at(kd.ga + q) * wa + ring_at(kd.gb + qb) * ((int32_t)DD_W / 2 - wa)) / ((int32_t)DD_W / 2);
        kd.pp = (q + 1u) % DD_W;
        f = kd.rf + kd.ratio;
        kd.ri += (uint32_t)(f >> 16);
        kd.rf = f & 0xFFFF;
        back = idx - kd.ri;
        if (back > PUNCH_N - 4u * DD_W) kd.ri = idx - 2u * DD_W;   /* too far behind: back near now */
        *wl = *wr = m;
        return;
    }
    case DD_GATE:
    case DD_BSKIP:
        if (kd.sub == DD_BSKIP && kd.mode == 2u) {     /* the 16th before, again */
            m = dd_declick(ring_at(kd.S + kd.k), kd.k, kd.L);
            if (++kd.k >= kd.L) kd.k = 0;
            *wl = *wr = m;
            return;
        }
        *wl = ((x >> 3) * (kd.gg >> 3)) >> 9;          /* (32 bits: no 64-bit product on the FM-1) */
        *wr = ((y >> 3) * (kd.gg >> 3)) >> 9;
        return;
    case DD_FILTER:
        *wl = x, *wr = y;                              /* (the filter runs on the block: kd_process) */
        return;
    case DD_RING: {
        int32_t s = sine_i(kd.ph);
        kd.ph += kd.dph;
        *wl = ((x >> 2) * (s >> 2)) >> 11;
        *wr = ((y >> 2) * (s >> 2)) >> 11;
        return;
    }
    case DD_FUZZ: {
        int32_t d = 256 + (int32_t)I * 40, v[2] = {x, y}, j;
        for (j = 0; j < 2; j++) {
            int32_t u = (clamp(v[j], -262143, 262143) * (d >> 2)) >> 6;
            if (u > 14000) u = 14000 + ((u - 14000) >> 3);   /* (the mix: 16-bit scale) */
            if (u < -14000) u = -14000 + ((u + 14000) >> 3);
            v[j] = clamp(u, -20000, 20000) * 2 / 5;     /* (the level about as the dry one) */
        }
        *wl = v[0], *wr = v[1];
        return;
    }
    case DD_GLITCH: {
        int32_t a = x, b = y, sh = 4 + (int32_t)I / 10;
        uint32_t hold = 1u + I / 6u;
        if (kd.corrupt) {                              /* a chunk of the past */
            a = b = ring_at(kd.csrc++);
            kd.corrupt--;
        }
        if (kd.noise) {
            kd.noise--;
            a = b = (int32_t)(kg_rand(&kd.rnd) >> 18) - 8192;   /* (a quarter of full scale) */
        }
        if (kd.hn++ % hold == 0u) kd.hl = a, kd.hr = b;
        *wl = (kd.hl >> sh) << sh;
        *wr = (kd.hr >> sh) << sh;
        return;
    }
    default:
        if (dd_isop(kd.sub)) {
            beat_sample(x, y, idx, wl, wr);
            return;
        }
        *wl = x, *wr = y;
        return;
    }
}

/* the effect that should play now, from kg_route (-1 none); fx.c, after punch_process */
static void kd_process1(int32_t *l, int32_t *r, uint32_t n, int32_t want);
/* the slots, one after the other (MULTI: the second destroys what the first made); fx.c, after punch_process */
static void kd_process(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t s;
    for (s = 0; s < KD_SLOTS; s++) {
        kdp = &kd_slot[s];
        kd_process1(l, r, n, kd_wantv[s]);
    }
    kdp = &kd_slot[0];
}
static void kd_process1(int32_t *l, int32_t *r, uint32_t n, int32_t want)
{
    uint32_t i, base = punch.w - n, s16, I;
    if (kd.cur < 0 && want < 0)
        return;
    if (kd.cur < 0) {                                  /* a new one */
        kd.cur = (int8_t)want;
        kd.g = 0;
        kd.t = 0;
        kd.s16 = 0xFFFFFFFFu;
        dd_engage(want == DD_RAND || want == DD_TOTAL ? DD_STUT : want, base);
    }
    s16 = song.playing ? clk_beat * 4u + clk_pos / ((uint32_t)FS * 15u) : kd.t / (dd_beat() / 4u + 1u);
    if (s16 != kd.s16) {
        kd.s16 = s16;
        dd_tick(base, s16);
    }
    I = dd_int();
    if (kd.sub == DD_GATE) {                           /* the slot of the bar: open or shut */
        uint32_t per = I > 50u ? 32u : 16u, slot = dd_barpos() * per / (dd_beat() * 4u + 1u);
        if (slot < kd.i32 || kd.i32 == 0xFFFFFFFFu) {  /* a new bar: a new pattern */
            uint32_t j;
            kd.mask = 0;
            for (j = 0; j < per; j++)
                if ((j % (per / 4u)) == 0u || kg_rand(&kd.rnd) % 100u >= 20u + I * 6u / 10u)
                    kd.mask |= 1u << j;
        }
        kd.i32 = slot;
    }
    if (dd_isgate(kd.sub))                             /* BEAT's gates: open or shut, on the bar */
        kd.open = (uint8_t)beat_gate(kd.sub, dd_barpos(), I);
    if (kd.sub == DD_FILTER) {                         /* the cutoff glides to its 16th's, per block */
        tsvf_t c;
        kd.cut += (kd.cut_to - kd.cut) / 8;
        tsvf_coef(&c, kd.cut, kd.reso);
        for (i = 0; i < n; i++) {
            l[i] += ((clamp(tsvf_lp(&c, l[i] >> 2, &kd.f1l, &kd.f2l) * 4 - l[i], -(1 << 20), 1 << 20) >> 3) * (kd.g >> 3)) >> 9;
            r[i] += ((clamp(tsvf_lp(&c, r[i] >> 2, &kd.f1r, &kd.f2r) * 4 - r[i], -(1 << 20), 1 << 20) >> 3) * (kd.g >> 3)) >> 9;
        }
    }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i], wl, wr, target = want == kd.cur ? 32767 : 0;
        if (kd.g < target) kd.g = kd.g + 512 > target ? target : kd.g + 512;
        else if (kd.g > target) kd.g = kd.g - 512 < target ? target : kd.g - 512;
        if (kd.sub == DD_GATE || kd.sub == DD_BSKIP || dd_isop(kd.sub)) {   /* the gate's gain glides (64 samples) */
            int32_t open = kd.sub == DD_GATE ? (kd.mask >> kd.i32 & 1u) : dd_isop(kd.sub) ? kd.open : kd.mode != 1u;
            int32_t gt = open ? 32767 : 0;
            kd.gg += gt > kd.gg ? (gt - kd.gg > 512 ? 512 : gt - kd.gg) : -(kd.gg - gt > 512 ? 512 : kd.gg - gt);
        }
        if (kd.sub == DD_FILTER) {
            kd.t++;
            continue;
        }
        dd_sample(x, y, base + i, &wl, &wr);
        if (kd.cur == DD_TOTAL) {                      /* TOTAL: crushed, sometimes ring modulated */
            int32_t sh = 6 + (int32_t)I / 14;
            wl = (wl >> sh) << sh, wr = (wr >> sh) << sh;
            if (kd.mode) {
                int32_t s = sine_i(kd.ph2);
                kd.ph2 += 600u * (0xFFFFFFFFu / FS);   /* (600 Hz) */
                wl = ((wl >> 2) * (s >> 2)) >> 11, wr = ((wr >> 2) * (s >> 2)) >> 11;
            }
        }
        if (kd.xf < 64) {                              /* a sub-effect just came in: from the dry sound */
            wl = x + (((wl - x) * kd.xf) >> 6);
            wr = y + (((wr - y) * kd.xf) >> 6);
            kd.xf++;
        }
        wl = clamp(wl - x, -(1 << 20), 1 << 20), wr = clamp(wr - y, -(1 << 20), 1 << 20);
        l[i] = clamp(x + (((wl >> 3) * (kd.g >> 3)) >> 9), -(1 << 21), (1 << 21) - 1);   /* (32 bits) */
        r[i] = clamp(y + (((wr >> 3) * (kd.g >> 3)) >> 9), -(1 << 21), (1 << 21) - 1);
        kd.t++;
    }
    if (kd.g == 0 && want != kd.cur) {                 /* faded out: the next one starts on the next block */
        kd.cur = -1;
        kd.own = 0;
    }
}
