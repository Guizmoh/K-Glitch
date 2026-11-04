/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef UI_SMALL                     /* (gfx.c: compiled for size, main loop only; the host tests without gfx.c: nothing) */
#define UI_SMALL
#endif
/* K-GLITCH: the DRUM DESTROYER layer over SLOOP's punch-in FX (punch.c). Included by fx.c after punch.c; runs in
 * the audio ISR (punch_process asks kg_route once a block which effect plays).
 *
 * Who picks the effect, highest first:
 *   1. a key (FX held + a key) or the FX CH's MIDI notes: punch.req, as in SLOOP;
 *   2. the effect sequencer (FX > D-SEQ, SEQ ON): one effect, or none, on each 16th of a bar, while PLAY runs;
 *   3. AUTO (FX > D-SETUP): on each 16th, DENSITY % of a chance to start a gesture, picked from a pool that CHAOS
 *      widens (the gentle loops and stutters first, then reverse / crush / half speed..., then tape stop, freeze,
 *      wobble...); MOVEMENT makes the gestures short (high) or long, up to a beat (low).
 * MUTATE (FX > D-SEQ) writes a new effect sequence from the same three macros; CHOP mutates the rhythm itself and
 * DRUMS writes one (kg_rhythm.c); CHAOS mode (FX > CHAOS) turns AUTO and the LFO on and mutates on the bar.
 * The effects: SLOOP's 27 punch-in ones (PX_*), K-GLITCH's DRUM DESTROYER bank (kg_dd.c, PUNCH_NFX + DD_*) and its
 * BEAT bank (kg_beat.c, PUNCH_NFX + 16 + n), one of which FX > BANK puts on the white keys and in AUTO / MUTATE; INTENSITY shapes them; HOLD keeps a key's effect after its
 * release (until another key or HOLD off).
 * The effects play on the FM-1's mix only (1.0: no MIDI CC out to an MPC any more; the tempo sync is SLOOP's). */

#define KGLITCH_VERSION "0.9.4"                     /* K-GLITCH's own version (SLOOP's: FELUCCA_VERSION, ui.c) */

#define KG_STEPS 64u                              /* the effect sequence / automation lane: up to 4 bars of 16ths */
/* the effects AUTO and MUTATE pick, gentle to wild: CHAOS 0 uses the first 4, 100 all of them (BANK SLOOP; BANK
 * DESTROY: KG_DPOOL, the DRUM DESTROYER ones) */
#define DDX(e) ((uint8_t)(27u + (e)))
static const uint8_t KG_DPOOL[] = {DDX(1), DDX(0), DDX(8), DDX(6), DDX(10), DDX(4), DDX(9), DDX(7), DDX(5), DDX(3),
                                   DDX(11), DDX(2), DDX(12), DDX(13), DDX(14), DDX(15)};   /* (DD_*: kg_dd.c) */
#define KG_NDPOOL (sizeof KG_DPOOL / sizeof KG_DPOOL[0])
/* BANK BEAT: its programs, gentle to wild (DD_RSTUT + n) */
static const uint8_t KG_BPOOL[] = {DDX(22), DDX(16), DDX(23), DDX(20), DDX(21), DDX(19), DDX(26), DDX(31), DDX(24),
                                   DDX(25), DDX(18), DDX(17), DDX(27), DDX(30), DDX(28), DDX(29)};
#define KG_NBPOOL (sizeof KG_BPOOL / sizeof KG_BPOOL[0])
static const uint8_t KG_POOL[] = {PX_LOOP16, PX_LOOP8, PX_STUT, PX_GATE,
                                  PX_LOOP32, PX_REV, PX_CRUSH, PX_LOOP4, PX_HALF, PX_LOOP8T, PX_DOWN, PX_ECHO, PX_RWD,
                                  PX_OCT, PX_STOP, PX_WOBBLE, PX_FREEZE, PX_FLANGE, PX_TEL, PX_KLOW, PX_KHIGH, PX_HPF,
                                  PX_LPF};
#define KG_NPOOL (sizeof KG_POOL / sizeof KG_POOL[0])
/* three-letter names for the sequencer's cells (ui_draw.c graph_kg) */
static const char *const KG_SHORT[PUNCH_NFX + 32u] = {
    "L4", "L8", "L16", "L32", "STU", "REV", "STP", "HLF", "LPF", "HPF", "TEL", "CRU", "ALI", "GAT", "ECH", "WOB",
    "KLO", "KMI", "KHI", "RWD", "OCT", "FLA", "PAN", "FRZ", "L8T", "MON", "FAD",
    /* the DRUM DESTROYER bank: four letters */
    "LOOP", "STUT", "GLIT", "SCRA", "REVS", "PTCH", "GATE", "FILT", "BREP", "SKIP", "MICR", "STRE", "RING", "FUZZ", "RAND",
    "TOTL",
    /* the BEAT bank */
    "RST", "RBR", "FIL", "ROL", "SK8", "STM", "GT8", "G16", "332", "RMP", "RBT", "ROF", "SBR", "SCT", "STP", "HLF"};

static struct {
    volatile int8_t seq[KG_STEPS];    /* main writes (D-SEQ, MUTATE), ISR reads and records: PX_*, PUNCH_NFX + DD_*, or -1 */
    volatile int8_t seq2[KG_STEPS];   /* the lane's second layer (MULTI: two effects recorded on a step) */
    volatile uint8_t sint[KG_STEPS];  /* each step's INTENSITY as recorded, + 1 (0: the effect's own) */
    volatile uint8_t prob[KG_STEPS];  /* each step's probability, 0..100 % */
    volatile uint8_t fnote[KG_STEPS]; /* the FILTER's automation: FREQ on the step (a note), 0 none */
    volatile uint8_t fmode[KG_STEPS]; /* .. TYPE + 1 (bits 0..3, 0 none), RESO / 8 (bits 4..7) */
    uint8_t pass;                     /* this 16th's step plays (its probability rolled) */
    volatile uint8_t nfx;             /* ISR: how many effects play now (MULTI) */
    volatile int8_t list[4];          /* .. the first four (the screen) */
    volatile uint8_t step;            /* ISR: the step of the sequence playing (0..len-1), for the screen */
    volatile int8_t now;              /* ISR: the effect asked for now (key, sequencer or AUTO), -1 none */
    volatile uint8_t src;             /* ISR: who asked: 0 none, 1 a key / MIDI, 2 the sequencer, 3 AUTO */
    uint32_t last16;                  /* the 16th last seen (beats * 4 + quarter), ~0 stopped */
    int8_t auto_fx;                   /* AUTO's gesture, -1 none */
    uint8_t auto_left;                /* its 16ths still to go */
    uint32_t rnd;
} kg = {.seq = {[0 ... KG_STEPS - 1] = -1}, .seq2 = {[0 ... KG_STEPS - 1] = -1}, .prob = {[0 ... KG_STEPS - 1] = 100},
        .now = -1, .last16 = 0xFFFFFFFFu, .auto_fx = -1, .rnd = 0x9E3779B9u};

static volatile int8_t kg_held[4] = {-1, -1, -1, -1};   /* MULTI: the pads held (seq.c key_down / key_up) */
static void kg_hold_add(int32_t fx)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) if (kg_held[i] == fx) return;
    for (i = 0; i < 4u; i++) if (kg_held[i] < 0) { kg_held[i] = (int8_t)fx; return; }
    kg_held[0] = kg_held[1], kg_held[1] = kg_held[2], kg_held[2] = kg_held[3], kg_held[3] = (int8_t)fx;   /* (the oldest goes) */
}
static void kg_hold_del(int32_t fx)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) if (kg_held[i] == fx) kg_held[i] = -1;
}
static void kg_hold_clear(void) { kg_held[0] = kg_held[1] = kg_held[2] = kg_held[3] = -1; }
static volatile uint8_t kf_touch;                 /* UI: the FILTER was touched (a knob, a key): REC writes it, else it
                                                   * takes over from the automation */
static volatile uint8_t kf_anote, kf_amode;       /* ISR: the automation in force (as fnote, fmode; 0 none) */
static volatile uint8_t kg_erase;                 /* UI: CLEAR (OCT- on the K-GLITCH pages) held: the steps passing are erased */
static volatile uint8_t kg_recd;                  /* ISR: an effect was recorded (the screen: REC FX) */
/* the effect sequence's length: SEQ OFF, 16, 32, 64 */
static uint32_t kg_seqlen(void)
{
    static const uint8_t LEN[4] = {0, 16, 32, 64};
    return LEN[(uint32_t)song.g[G_KSEQ] & 3u];
}
static uint32_t kg_rand(uint32_t *s)              /* xorshift32 */
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x ? x : 0x2545F491u;
}
static uint32_t kg_pct(int32_t g) { return (uint32_t)(song.g[g] < 0 ? 0 : song.g[g] > 100 ? 100 : song.g[g]); }
#include "kg_dd.c"                              /* the DRUM DESTROYER bank: ids PUNCH_NFX .. KG_NFX - 1 */
static void kg_mutate(uint32_t seed);
#include "kg_rhythm.c"                          /* DRUMS (the generator) and CHOP (the rhythm's mutations) */
#include "kg_loop.c"                            /* LOOP: a loop recorded from what plays, sliced, as grains */
#include "kg_filter.c"                          /* FILTER: LP HP BP NOTCH DRIVE COMB METAL VOWEL PHASER RESONATOR */
static const char *kg_name(int32_t fx) { return fx < (int32_t)PUNCH_NFX ? PUNCH_NAME[fx] : DD_NAME[fx - (int32_t)PUNCH_NFX]; }
/* the bank on the white keys: its first effect's id (BANK SLOOP: 0, its own PX_*) */
static int32_t kg_bank_base(void)
{
    uint32_t b = (uint32_t)song.g[G_KBANK];
    return b ? (int32_t)(PUNCH_NFX + (b >= 2u ? DD_BANK : 0u)) : 0;
}
/* FX held + a key: SLOOP's effect, or with BANK on DESTROY / BEAT a white key's effect of that bank (seq.c key_down,
 * midi_fx) */
static int32_t kg_fx_of_key(uint32_t k)
{
    int32_t fx = punch_fx_of_key(k);
    return song.g[G_KBANK] && fx >= 0 && fx < 16 ? kg_bank_base() + fx : fx;
}
/* an effect from the CHAOS pool */
static int8_t kg_pick(uint32_t *s)
{
    if (song.g[G_KBANK] >= 2) {
        uint32_t n = 3u + kg_pct(G_KCHAOS) * (KG_NBPOOL - 3u) / 100u;
        return (int8_t)KG_BPOOL[kg_rand(s) % n];
    }
    if (song.g[G_KBANK]) {
        uint32_t n = 3u + kg_pct(G_KCHAOS) * (KG_NDPOOL - 3u) / 100u;
        return (int8_t)KG_DPOOL[kg_rand(s) % n];
    }
    {
        uint32_t n = 4u + kg_pct(G_KCHAOS) * (KG_NPOOL - 4u) / 100u;
        return (int8_t)KG_POOL[kg_rand(s) % n];
    }
}
/* MOVEMENT: a gesture's length in 16ths, 1 (MOVEMENT 100) up to 4 (MOVEMENT 0) */
static uint32_t kg_len(uint32_t *s)
{
    uint32_t most = 1u + (100u - kg_pct(G_KMOVE)) * 3u / 100u;
    return 1u + kg_rand(s) % most;
}

/* REC (ISR): the effect lane at least as long as the loop playing (its longest track: 16, 32 or 64 steps), so a 2- or
 * 4-bar loop records its effects on all its bars. A lane made longer repeats what it had (as it played), so nothing
 * recorded is lost (off: the lane as it was kept, CLEAN keeps it); never shortened here (FX > SEQ, K4 does that) */
static void kg_rec_len(void)
{
    uint32_t i, loop = 0, len = kg_seqlen(), want;
    for (i = 0; i < NTRK; i++)
        if ((uint32_t)trk[i].p[P_SLEN] > loop) loop = (uint32_t)trk[i].p[P_SLEN];
    want = loop > 32u ? 3u : loop > 16u ? 2u : 1u;
    if (len && (uint32_t)song.g[G_KSEQ] >= want)
        return;
    if (len)                                          /* longer: the steps so far again, as they played */
        for (i = len; i < KG_STEPS; i++) {
            kg.seq[i] = kg.seq[i % len], kg.seq2[i] = kg.seq2[i % len], kg.sint[i] = kg.sint[i % len];
            kg.prob[i] = kg.prob[i % len], kg.fnote[i] = kg.fnote[i % len], kg.fmode[i] = kg.fmode[i % len];
        }
    song.g[G_KSEQ] = (int16_t)(len && (uint32_t)song.g[G_KSEQ] > want ? song.g[G_KSEQ] : want);
    if (kg.last16 != 0xFFFFFFFFu)
        kg.step = (uint8_t)(kg.last16 % kg_seqlen());   /* (this 16th, in the longer lane) */
}

/* once a block, from punch_process: the effect the mix plays (-1 none) given the keys' (key) */
static int32_t kg_route(int32_t key)
{
    int32_t fx = key;
    uint32_t autos = song.g[G_KAUTO] || song.g[G_KCMODE];   /* (CHAOS mode: AUTO and the LFO on) */
    {   /* the LFO: MOVEMENT its depth (up to 50 points of INTENSITY) and its speed (4 bars .. a beat), on the transport */
        static const uint8_t BEATS[5] = {16, 8, 4, 2, 1};
        static uint32_t free_ph;
        uint32_t mv = kg_pct(G_KMOVE), nb = BEATS[mv * 5u / 101u], ph;
        if (song.playing) {
            uint32_t q = clk_pos / 41u;                   /* (FS * 60 units a beat: ~65536ths) */
            ph = (((clk_beat % nb) << 16) + (q > 65535u ? 65535u : q)) / nb << 16;
        } else
            ph = free_ph += (0xFFFFFFFFu / ((uint32_t)FS / CTL * 2u)) / nb;   /* (stopped: as at 120 BPM) */
        kd_lfo = (int8_t)(song.g[G_KLFO] || song.g[G_KCMODE] ? sine_i(ph) * (int32_t)(mv / 2u) / 32767 : 0);
        kg_lfo_ph = ph;                               /* (the FILTER's MOVE rides the same LFO) */
    }
    if (song.playing)
        kr_bar();                                     /* CHOP EVERY, CHAOS mode: the rhythm mutates on the bar */
    else
        kr.bar = 0xFFFFFFFFu;
    if (song.playing) {
        uint32_t s16 = clk_beat * 4u + clk_pos / ((uint32_t)FS * 15u);   /* (clk_pos: FS * 60 units a beat) */
        if (s16 != kg.last16) {                       /* a new 16th */
            uint32_t len = kg_seqlen();
            kg.last16 = s16;
            kg.step = (uint8_t)(s16 % (len ? len : 16u));
            kg.pass = (uint8_t)(kg_rand(&kg.rnd) % 100u < kg.prob[kg.step]);   /* (the step's probability) */
            if (kg.fnote[kg.step] && !kg_erase && !kf_touch)   /* the FILTER's automation: from this step on */
                kf_anote = kg.fnote[kg.step], kf_amode = kg.fmode[kg.step];
            if (kg.auto_left)
                kg.auto_left--;
            if (!kg.auto_left) {
                kg.auto_fx = -1;
                if (autos && kg_rand(&kg.rnd) % 100u < kg_pct(G_KDENS)) {
                    kg.auto_fx = kg_pick(&kg.rnd);
                    kg.auto_left = (uint8_t)kg_len(&kg.rnd);
                }
            }
        }
    } else {
        kg.last16 = 0xFFFFFFFFu;
        kg.auto_fx = -1;
        kg.auto_left = 0;
        kf_anote = kf_amode = 0;                      /* (stopped: the knobs) */
    }
    if (song.playing && kg_erase) {                   /* CLEAR: the FILTER's automation of the steps passing too */
        kg.fnote[kg.step] = kg.fmode[kg.step] = 0;
    } else if (kf_touch) {                            /* the FILTER touched: REC writes it; else the knobs take over */
        if (song.playing && song.rec) {
            kg.fnote[kg.step] = (uint8_t)song.g[G_KFCUT];
            kg.fmode[kg.step] = (uint8_t)(((uint32_t)song.g[G_KFTYPE] + 1u) | ((uint32_t)song.g[G_KFRES] >> 3) << 4);
            kg_recd = 1;
            kg_rec_len();
        }
        kf_anote = kf_amode = 0;
        kf_touch = 0;
    }
    /* what plays now, first first: SOLO one effect (a key, else the lane, else AUTO); MULTI all of them stacked (the
     * keys held, the lane's two layers, AUTO). SLOOP's effects: one at a time (punch.c); DRUM DESTROYER: KD_SLOTS */
    {
        int8_t lst[8], ssrc[8], sint[8];
        uint32_t nl = 0, multi = song.g[G_KMULTI] != 0, k, s, used = 0;
        int32_t sloop = -1, A = -1, B = -1;
        int8_t nw[KD_SLOTS];
        uint8_t nsi[KD_SLOTS];
#define KG_ADD(id, src_, si_) do { int32_t _i = (id); uint32_t _j; \
            if (_i < 0 || _i >= (int32_t)KG_NFX || nl >= 8u) break; \
            for (_j = 0; _j < nl && lst[_j] != _i; _j++) {} \
            if (_j == nl) { lst[nl] = (int8_t)_i; ssrc[nl] = (int8_t)(src_); sint[nl] = (int8_t)(si_); nl++; } } while (0)
        if (multi)
            for (k = 0; k < 4u; k++) KG_ADD(kg_held[k], 1, 0);
        KG_ADD(fx, 1, 0);
        if (song.playing && song.rec && nl)
            kg_rec_len();                             /* (REC: the lane as long as the loop) */
        if (song.playing && song.g[G_KSEQ]) {
            if (kg_erase) {                           /* CLEAR held: the steps passing are erased (as the PO-33) */
                kg.seq[kg.step] = kg.seq2[kg.step] = -1;
                kg.sint[kg.step] = 0;
                kg.prob[kg.step] = 100;
            } else if (nl && song.rec) {              /* REC: the pads' effects written on their 16ths (two layers) */
                kg.seq[kg.step] = lst[0];
                kg.seq2[kg.step] = (int8_t)(multi && nl > 1u ? lst[1] : -1);
                kg.sint[kg.step] = (uint8_t)(lst[0] >= (int32_t)PUNCH_NFX ? kd_int[(lst[0] - (int32_t)PUNCH_NFX) % DD_N] + 1u : 0u);
                kg.prob[kg.step] = 100;
                kg_recd = 1;
            } else if (kg.pass) {                     /* the lane, if its probability let it */
                A = kg.seq[kg.step];
                B = kg.seq2[kg.step];
            }
        }
        if (!nl || multi) {
            KG_ADD(A, 2, kg.sint[kg.step]);
            if (multi) KG_ADD(B, 2, 0);
        }
        if ((!nl || multi) && !kg_erase)
            KG_ADD(kg.auto_fx, 3, 0);
#undef KG_ADD
        kg.now = (int8_t)(nl ? lst[0] : -1);
        kg.src = (uint8_t)(nl ? ssrc[0] : 0);
        kg.nfx = (uint8_t)nl;
        for (k = 0; k < 4u; k++) kg.list[k] = (int8_t)(k < nl ? lst[k] : -1);
        /* SLOOP's effect: the first; DRUM DESTROYER's: a slot each (one already playing it keeps it) */
        for (s = 0; s < KD_SLOTS; s++) nw[s] = -1, nsi[s] = 0;
        for (k = 0; k < nl; k++) {
            int32_t id = lst[k];
            if (id < (int32_t)PUNCH_NFX) {
                if (sloop < 0) sloop = id;
                continue;
            }
            for (s = 0; s < KD_SLOTS; s++)
                if (kd_slot[s].cur == id - (int32_t)PUNCH_NFX && nw[s] < 0) break;
            if (s == KD_SLOTS)
                for (s = 0; s < KD_SLOTS; s++)
                    if (nw[s] < 0 && kd_slot[s].cur < 0) break;
            if (s == KD_SLOTS)
                for (s = 0; s < KD_SLOTS; s++)
                    if (nw[s] < 0) break;
            if (s == KD_SLOTS) continue;
            nw[s] = (int8_t)(id - (int32_t)PUNCH_NFX);
            nsi[s] = (uint8_t)sint[k];
            used++;
        }
        for (s = 0; s < KD_SLOTS; s++) {
            kd_wantv[s] = nw[s];
            kd_sintv[s] = nsi[s];
        }
        (void)used;
        fx = sloop;
    }
    return fx;                                        /* SLOOP's effect, for punch.c (-1 none) */
}

/* MUTATE (main loop): a new effect sequence. DENSITY: how many steps; CHAOS: which effects, and how often a step
 * repeats the one before (a fragment carried on) instead of a new one; the first step of each beat stays dry
 * unless CHAOS is past 70 % (the groove stays readable) */
static void kg_mutate(uint32_t seed)
{
    static uint32_t s = 0x1234567u;
    uint32_t i, dens = kg_pct(G_KDENS), chaos = kg_pct(G_KCHAOS), len = kg_seqlen();
    s ^= seed * 2654435761u;
    if (!len) {                                       /* (SEQ OFF: DICE turns it on, 16 steps) */
        song.g[G_KSEQ] = 1;
        len = 16;
    }
    for (i = 0; i < KG_STEPS; i++) {
        kg.sint[i] = 0;
        kg.seq2[i] = -1;
        kg.prob[i] = 100;
    }
    for (i = 0; i < len; i++) {
        int8_t fx = -1;
        if ((i & 3u) == 0u && chaos <= 70u)
            fx = -1;
        else if (kg_rand(&s) % 100u < dens)
            fx = i && kg.seq[i - 1u] >= 0 && kg_rand(&s) % 100u < chaos / 2u ? kg.seq[i - 1u] : kg_pick(&s);
        kg.seq[i] = fx;
        if (fx >= 0 && chaos > 50u && kg_rand(&s) % 100u < chaos - 50u)
            kg.prob[i] = (uint8_t)(40u + kg_rand(&s) % 50u);   /* (CHAOS past 50 %: some steps only maybe) */
    }
}
static void kg_clear(void)
{
    uint32_t i;
    for (i = 0; i < KG_STEPS; i++) {
        kg.seq[i] = kg.seq2[i] = -1;
        kg.sint[i] = 0;
        kg.prob[i] = 100;
        kg.fnote[i] = kg.fmode[i] = 0;
    }
}
/* CLEAR (FX > DESTROY, KNOB 4): back to the original sound. Every effect let go (a held one too), AUTO, the effect
 * sequencer, CHOP's EVERY and CHAOS mode off, the rhythm back to its ORIGINAL; the effect sequence itself is kept
 * (SEQ ON plays it again). Main loop */
static void kg_clean(void)
{
    song.g[G_KHOLD] = song.g[G_KAUTO] = song.g[G_KEVERY] = song.g[G_KCMODE] = 0;
    kg_hold_clear();
    song.g[G_KSEQ] = 0;                              /* (the automation stays: SEQ 16 / 32 / 64 plays it again) */
    punch.req = -1;
    punch.keybit = 0;
    fm1_irq_off();
    kr_restore();
    fm1_irq_on();
}


/* ---- the K-GLITCH state kept in flash (storage.c's tail record of the settings object; project.c saves it 3 s after a
 * change, stopped, and takes it back at power-on): the effect lane (both layers, intensities, probabilities), each
 * effect's INTENSITY, the slices' map, and K-GLITCH's settings. The loop's sound itself is RAM only */
#define ST_TAIL_MAX_KG 2048u                     /* (storage.c ST_TAIL_MAX) */
#define KG_BLOB_MAGIC 0x344C474Bu                 /* "KGL4" (0.8: the BEAT bank's intensities, the drum kits) */
typedef struct {
    uint32_t magic;
    uint16_t ng, ver;
    int8_t seq[KG_STEPS], seq2[KG_STEPS];
    uint8_t sint[KG_STEPS], prob[KG_STEPS], fnote[KG_STEPS], fmode[KG_STEPS];
    uint8_t dint[DD_N], map[KL_NS], rev[KL_NS], ret[KL_NS];
    int16_t g[G_COUNT - G_KCHAOS];
    kgd_snd_t kit[KGD_NK][DRUM_LANES];          /* the K-GLITCH drum kits (kg_drum.c) */
    uint32_t sum;
} kg_blob_t;
_Static_assert(sizeof(kg_blob_t) <= ST_TAIL_MAX_KG, "the tail record holds it");
static kg_blob_t kg_blob;
static uint32_t kg_blob_sum(const kg_blob_t *b)
{
    const uint8_t *p = (const uint8_t *)b;
    uint32_t h = 2166136261u, i;
    for (i = 0; i < sizeof *b - 4u; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}
UI_SMALL static void kg_pack(kg_blob_t *b)                 /* (main loop) */
{
    uint32_t i;
    memset(b, 0, sizeof *b);
    b->magic = KG_BLOB_MAGIC;
    b->ng = (uint16_t)(G_COUNT - G_KCHAOS);
    b->ver = 1;
    for (i = 0; i < KG_STEPS; i++)
        b->seq[i] = kg.seq[i], b->seq2[i] = kg.seq2[i], b->sint[i] = kg.sint[i], b->prob[i] = kg.prob[i],
        b->fnote[i] = kg.fnote[i], b->fmode[i] = kg.fmode[i];
    for (i = 0; i < DD_N; i++) b->dint[i] = kd_int[i];
    for (i = 0; i < KL_NS; i++) b->map[i] = kl.map[i], b->rev[i] = kl.rev[i], b->ret[i] = kl.ret[i];
    for (i = 0; i < (uint32_t)(G_COUNT - G_KCHAOS); i++) b->g[i] = song.g[G_KCHAOS + i];
    (void)kgd_snd(0, 0);                              /* (set up) */
    memcpy(b->kit, kgd_kit, sizeof b->kit);
    b->sum = kg_blob_sum(b);
}
UI_SMALL static int kg_unpack(const kg_blob_t *b)          /* 0: taken, -1: not one (nothing changed) */
{
    uint32_t i, ng;
    if (b->magic != KG_BLOB_MAGIC || b->sum != kg_blob_sum(b))
        return -1;
    for (i = 0; i < KG_STEPS; i++) {
        kg.seq[i] = b->seq[i] < (int8_t)KG_NFX ? b->seq[i] : -1;
        kg.seq2[i] = b->seq2[i] < (int8_t)KG_NFX ? b->seq2[i] : -1;
        kg.sint[i] = b->sint[i] <= 101u ? b->sint[i] : 0u;
        kg.prob[i] = b->prob[i] <= 100u ? b->prob[i] : 100u;
        kg.fnote[i] = b->fnote[i] >= 24u && b->fnote[i] <= 120u ? b->fnote[i] : 0u;
        kg.fmode[i] = kg.fnote[i] && (b->fmode[i] & 15u) <= KF_N ? b->fmode[i] : 0u;
    }
    for (i = 0; i < DD_N; i++) kd_int[i] = b->dint[i] <= 100u ? b->dint[i] : 50u;
    for (i = 0; i < KL_NS; i++) {
        kl.map[i] = (uint8_t)(b->map[i] % KL_NS);
        kl.rev[i] = (uint8_t)(b->rev[i] != 0);
        kl.ret[i] = b->ret[i] >= 1u && b->ret[i] <= 8u ? b->ret[i] : 1u;
    }
    ng = b->ng < (uint32_t)(G_COUNT - G_KCHAOS) ? b->ng : (uint32_t)(G_COUNT - G_KCHAOS);   /* (an older record: fewer) */
    for (i = 0; i < ng; i++) {
        uint32_t id = G_KCHAOS + i;
        if (id == G_KLMODE) continue;              /* (no loop at power-on: its sound is RAM only) */
        song.g[id] = (int16_t)clamp(b->g[i], GP[id].min, GP[id].max);
    }
    memcpy(kgd_kit, b->kit, sizeof kgd_kit);
    kgd_ready = 1;
    for (i = 0; i < KGD_NK * DRUM_LANES; i++)
        kgd_fix(&kgd_kit[i / DRUM_LANES][i % DRUM_LANES], i / DRUM_LANES);
    return 0;
}
