/* SPDX-License-Identifier: GPL-3.0-only */
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
 * The effects: SLOOP's 27 punch-in ones (PX_*) and K-GLITCH's DRUM DESTROYER bank (kg_dd.c, PUNCH_NFX + DD_*), which
 * FX > BANK puts on the white keys and in AUTO / MUTATE; INTENSITY shapes them; HOLD keeps a key's effect after its
 * release (until another key or HOLD off).
 *
 * Where it plays (FX > DESTROY, OUT): FM-1 (the FM-1's mix, as SLOOP), MPC (only a MIDI CC goes out: the FM-1's mix
 * stays dry) or BOTH. With MPC or BOTH, each effect is a CC on the D-SETUP channel (16 by default): 127 when it
 * starts, 0 when it stops, one at a time; CHAOS, DENSITY and MOVEMENT go out as CC 85, 86, 87 (0..127) when they
 * change. On the MPC each CC is set with MIDI LEARN. The CC map is in docs/MPC_REMOTE.md (K-GLITCH's root). */

#define KGLITCH_VERSION "0.3"                     /* K-GLITCH's own version (SLOOP's: FELUCCA_VERSION, ui.c) */
static void midi_out_event(uint32_t pkt);         /* usb.c: a USB-MIDI packet, from the audio ISR */

#define KG_STEPS 16u
enum { KG_OUT_FM1, KG_OUT_MPC, KG_OUT_BOTH };
/* the CC of each punch-in effect (PX_*): 20..31 then 102..116, clear of bank select, modulation, volume, pan,
 * sustain, the RPN / NRPN and the channel modes */
static const uint8_t KG_CC[PUNCH_NFX] = {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                                         102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116};
#define KG_CC_CHAOS 85u
#define KG_CC_DENS 86u
#define KG_CC_MOVE 87u
#define KG_CC_INT 89u                             /* INTENSITY (FX > BANK) */
/* the effects AUTO and MUTATE pick, gentle to wild: CHAOS 0 uses the first 4, 100 all of them (BANK SLOOP; BANK
 * DESTROY: KG_DPOOL, the DRUM DESTROYER ones) */
#define DDX(e) ((uint8_t)(27u + (e)))
static const uint8_t KG_DPOOL[] = {DDX(1), DDX(0), DDX(8), DDX(6), DDX(10), DDX(4), DDX(9), DDX(7), DDX(5), DDX(3),
                                   DDX(11), DDX(2), DDX(12), DDX(13), DDX(14), DDX(15)};   /* (DD_*: kg_dd.c) */
#define KG_NDPOOL (sizeof KG_DPOOL / sizeof KG_DPOOL[0])
static const uint8_t KG_POOL[] = {PX_LOOP16, PX_LOOP8, PX_STUT, PX_GATE,
                                  PX_LOOP32, PX_REV, PX_CRUSH, PX_LOOP4, PX_HALF, PX_LOOP8T, PX_DOWN, PX_ECHO, PX_RWD,
                                  PX_OCT, PX_STOP, PX_WOBBLE, PX_FREEZE, PX_FLANGE, PX_TEL, PX_KLOW, PX_KHIGH, PX_HPF,
                                  PX_LPF};
#define KG_NPOOL (sizeof KG_POOL / sizeof KG_POOL[0])
/* three-letter names for the sequencer's cells (ui_draw.c graph_kg) */
static const char *const KG_SHORT[PUNCH_NFX + 16u] = {
    "L4", "L8", "L16", "L32", "STU", "REV", "STP", "HLF", "LPF", "HPF", "TEL", "CRU", "ALI", "GAT", "ECH", "WOB",
    "KLO", "KMI", "KHI", "RWD", "OCT", "FLA", "PAN", "FRZ", "L8T", "MON", "FAD",
    /* the DRUM DESTROYER bank: four letters */
    "LOOP", "STUT", "GLIT", "SCRA", "REVS", "PTCH", "GATE", "FILT", "BREP", "SKIP", "MICR", "STRE", "RING", "FUZZ", "RAND",
    "TOTL"};

static struct {
    volatile int8_t seq[KG_STEPS];    /* main writes (D-SEQ, MUTATE), ISR reads: PX_*, PUNCH_NFX + DD_*, or -1 */
    volatile uint8_t step;            /* ISR: the 16th playing (0..15), for the screen */
    volatile int8_t now;              /* ISR: the effect asked for now (key, sequencer or AUTO), -1 none */
    volatile uint8_t src;             /* ISR: who asked: 0 none, 1 a key / MIDI, 2 the sequencer, 3 AUTO */
    uint32_t last16;                  /* the 16th last seen (beats * 4 + quarter), ~0 stopped */
    int8_t auto_fx;                   /* AUTO's gesture, -1 none */
    uint8_t auto_left;                /* its 16ths still to go */
    int8_t sent_fx;                   /* the effect whose CC is at 127, -1 none */
    uint8_t sent_ch;                  /* the channel it went out on (0..15) */
    int16_t sent_mac[4];              /* the macros (and INTENSITY) as last sent (-1: not yet) */
    uint32_t rnd;
} kg = {.seq = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        .now = -1, .last16 = 0xFFFFFFFFu, .auto_fx = -1, .sent_fx = -1, .sent_mac = {-1, -1, -1, -1}, .rnd = 0x9E3779B9u};

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
/* an effect's CC: its key's (DRUM DESTROYER n is on SLOOP's white key n: the same CC, the MPC maps keys) */
static uint32_t kg_cc_of(int32_t fx) { return KG_CC[fx < (int32_t)PUNCH_NFX ? fx : fx - (int32_t)PUNCH_NFX]; }
static const char *kg_name(int32_t fx) { return fx < (int32_t)PUNCH_NFX ? PUNCH_NAME[fx] : DD_NAME[fx - (int32_t)PUNCH_NFX]; }
/* FX held + a key: SLOOP's effect, or with BANK on DESTROY a white key's DRUM DESTROYER effect (seq.c key_down, midi_fx) */
static int32_t kg_fx_of_key(uint32_t k)
{
    int32_t fx = punch_fx_of_key(k);
    return song.g[G_KBANK] && fx >= 0 && fx < 16 ? (int32_t)PUNCH_NFX + fx : fx;
}
/* an effect from the CHAOS pool */
static int8_t kg_pick(uint32_t *s)
{
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

static void kg_cc(uint32_t ch, uint32_t cc, uint32_t v)
{
    midi_out_event(0x0Bu | (0xB0u | (ch & 15u)) << 8 | (cc & 127u) << 16 | (v & 127u) << 24);
}

/* once a block, from punch_process: the effect the mix plays (-1 none) given the keys' (key) */
static int32_t kg_route(int32_t key)
{
    int32_t fx = key, out = (int32_t)song.g[G_KTGT], ch = (int32_t)song.g[G_KCCCH];
    uint32_t i, autos = song.g[G_KAUTO] || song.g[G_KCMODE];   /* (CHAOS mode: AUTO and the LFO on) */
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
    }
    if (song.playing)
        kr_bar();                                     /* CHOP EVERY, CHAOS mode: the rhythm mutates on the bar */
    else
        kr.bar = 0xFFFFFFFFu;
    if (song.playing) {
        uint32_t s16 = clk_beat * 4u + clk_pos / ((uint32_t)FS * 15u);   /* (clk_pos: FS * 60 units a beat) */
        if (s16 != kg.last16) {                       /* a new 16th */
            kg.last16 = s16;
            kg.step = (uint8_t)(s16 & (KG_STEPS - 1u));
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
    }
    kg.src = fx >= 0 ? 1u : 0u;
    if (fx < 0 && song.playing && song.g[G_KSEQ] && kg.seq[kg.step] >= 0 && kg.seq[kg.step] < (int8_t)KG_NFX) {
        fx = kg.seq[kg.step];
        kg.src = 2;
    }
    if (fx < 0 && kg.auto_fx >= 0) {
        fx = kg.auto_fx;
        kg.src = 3;
    }
    kg.now = (int8_t)fx;
    /* the MPC: one CC at 127 at a time; anything else (FM-1, the channel OFF or changed) lets it go first */
    if (out == KG_OUT_FM1 || ch <= 0)
        fx = -1, ch = 0;
    if (kg.sent_fx >= 0 && (kg.now != kg.sent_fx || fx < 0 || (uint32_t)(ch - 1) != kg.sent_ch)) {
        kg_cc(kg.sent_ch, kg_cc_of(kg.sent_fx), 0);
        kg.sent_fx = -1;
    }
    if (fx >= 0 && kg.sent_fx < 0) {
        kg.sent_ch = (uint8_t)(ch - 1);
        kg.sent_fx = (int8_t)fx;
        kg_cc(kg.sent_ch, kg_cc_of(fx), 127);
    }
    if (ch > 0)                                       /* the macros, when they change */
        for (i = 0; i < 4u; i++) {
            int16_t v = (int16_t)(kg_pct(i < 3u ? (int32_t)(G_KCHAOS + i) : (int32_t)G_KINT) * 127u / 100u);
            if (v != kg.sent_mac[i]) {
                kg.sent_mac[i] = v;
                kg_cc((uint32_t)ch - 1u, i < 3u ? KG_CC_CHAOS + i : KG_CC_INT, (uint32_t)v);
            }
        }
    else
        kg.sent_mac[0] = kg.sent_mac[1] = kg.sent_mac[2] = kg.sent_mac[3] = -1;
    return out == KG_OUT_MPC ? -1 : kg.now;           /* MPC: the FM-1's mix stays dry */
}

/* MUTATE (main loop): a new effect sequence. DENSITY: how many steps; CHAOS: which effects, and how often a step
 * repeats the one before (a fragment carried on) instead of a new one; the first step of each beat stays dry
 * unless CHAOS is past 70 % (the groove stays readable) */
static void kg_mutate(uint32_t seed)
{
    static uint32_t s = 0x1234567u;
    uint32_t i, dens = kg_pct(G_KDENS), chaos = kg_pct(G_KCHAOS);
    s ^= seed * 2654435761u;
    for (i = 0; i < KG_STEPS; i++) {
        int8_t fx = -1;
        if ((i & 3u) == 0u && chaos <= 70u)
            fx = -1;
        else if (kg_rand(&s) % 100u < dens)
            fx = i && kg.seq[i - 1u] >= 0 && kg_rand(&s) % 100u < chaos / 2u ? kg.seq[i - 1u] : kg_pick(&s);
        kg.seq[i] = fx;
    }
}
static void kg_clear(void)
{
    uint32_t i;
    for (i = 0; i < KG_STEPS; i++)
        kg.seq[i] = -1;
}
/* CLEAR (FX > DESTROY, KNOB 4): back to the original sound. Every effect let go (a held one too), AUTO, the effect
 * sequencer, CHOP's EVERY and CHAOS mode off, the rhythm back to its ORIGINAL; the effect sequence itself is kept
 * (SEQ ON plays it again). Main loop */
static void kg_clean(void)
{
    song.g[G_KHOLD] = song.g[G_KAUTO] = song.g[G_KSEQ] = song.g[G_KEVERY] = song.g[G_KCMODE] = 0;
    punch.req = -1;
    punch.keybit = 0;
    fm1_irq_off();
    kr_restore();
    fm1_irq_on();
}

