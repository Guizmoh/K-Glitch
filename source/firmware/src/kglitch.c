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
 * MUTATE (FX > D-SEQ) writes a new effect sequence from the same three macros.
 *
 * Where it plays (FX > DESTROY, OUT): FM-1 (the FM-1's mix, as SLOOP), MPC (only a MIDI CC goes out: the FM-1's mix
 * stays dry) or BOTH. With MPC or BOTH, each effect is a CC on the D-SETUP channel (16 by default): 127 when it
 * starts, 0 when it stops, one at a time; CHAOS, DENSITY and MOVEMENT go out as CC 85, 86, 87 (0..127) when they
 * change. On the MPC each CC is set with MIDI LEARN. The CC map is in docs/MPC_REMOTE.md (K-GLITCH's root). */

#define KGLITCH_VERSION "0.1"                     /* K-GLITCH's own version (SLOOP's: FELUCCA_VERSION, ui.c) */
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
/* the effects AUTO and MUTATE pick, gentle to wild: CHAOS 0 uses the first 4, 100 all of them */
static const uint8_t KG_POOL[] = {PX_LOOP16, PX_LOOP8, PX_STUT, PX_GATE,
                                  PX_LOOP32, PX_REV, PX_CRUSH, PX_LOOP4, PX_HALF, PX_LOOP8T, PX_DOWN, PX_ECHO, PX_RWD,
                                  PX_OCT, PX_STOP, PX_WOBBLE, PX_FREEZE, PX_FLANGE, PX_TEL, PX_KLOW, PX_KHIGH, PX_HPF,
                                  PX_LPF};
#define KG_NPOOL (sizeof KG_POOL / sizeof KG_POOL[0])
/* three-letter names for the sequencer's cells (ui_draw.c graph_kg) */
static const char *const KG_SHORT[PUNCH_NFX] = {
    "L4", "L8", "L16", "L32", "STU", "REV", "STP", "HLF", "LPF", "HPF", "TEL", "CRU", "ALI", "GAT", "ECH", "WOB",
    "KLO", "KMI", "KHI", "RWD", "OCT", "FLA", "PAN", "FRZ", "L8T", "MON", "FAD"};

static struct {
    volatile int8_t seq[KG_STEPS];    /* main writes (D-SEQ, MUTATE), ISR reads: PX_* or -1 */
    volatile uint8_t step;            /* ISR: the 16th playing (0..15), for the screen */
    volatile int8_t now;              /* ISR: the effect asked for now (key, sequencer or AUTO), -1 none */
    volatile uint8_t src;             /* ISR: who asked: 0 none, 1 a key / MIDI, 2 the sequencer, 3 AUTO */
    uint32_t last16;                  /* the 16th last seen (beats * 4 + quarter), ~0 stopped */
    int8_t auto_fx;                   /* AUTO's gesture, -1 none */
    uint8_t auto_left;                /* its 16ths still to go */
    int8_t sent_fx;                   /* the effect whose CC is at 127, -1 none */
    uint8_t sent_ch;                  /* the channel it went out on (0..15) */
    int16_t sent_mac[3];              /* the macros as last sent (-1: not yet) */
    uint32_t rnd;
} kg = {.seq = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        .now = -1, .last16 = 0xFFFFFFFFu, .auto_fx = -1, .sent_fx = -1, .sent_mac = {-1, -1, -1}, .rnd = 0x9E3779B9u};

static uint32_t kg_rand(uint32_t *s)              /* xorshift32 */
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x ? x : 0x2545F491u;
}
static uint32_t kg_pct(int32_t g) { return (uint32_t)(song.g[g] < 0 ? 0 : song.g[g] > 100 ? 100 : song.g[g]); }
/* an effect from the CHAOS pool */
static int8_t kg_pick(uint32_t *s)
{
    uint32_t n = 4u + kg_pct(G_KCHAOS) * (KG_NPOOL - 4u) / 100u;
    return (int8_t)KG_POOL[kg_rand(s) % n];
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
    uint32_t i;
    if (song.playing) {
        uint32_t s16 = clk_beat * 4u + clk_pos / ((uint32_t)FS * 15u);   /* (clk_pos: FS * 60 units a beat) */
        if (s16 != kg.last16) {                       /* a new 16th */
            kg.last16 = s16;
            kg.step = (uint8_t)(s16 & (KG_STEPS - 1u));
            if (kg.auto_left)
                kg.auto_left--;
            if (!kg.auto_left) {
                kg.auto_fx = -1;
                if (song.g[G_KAUTO] && kg_rand(&kg.rnd) % 100u < kg_pct(G_KDENS)) {
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
    if (fx < 0 && song.playing && song.g[G_KSEQ] && kg.seq[kg.step] >= 0 && kg.seq[kg.step] < (int8_t)PUNCH_NFX) {
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
        kg_cc(kg.sent_ch, KG_CC[kg.sent_fx], 0);
        kg.sent_fx = -1;
    }
    if (fx >= 0 && kg.sent_fx < 0) {
        kg.sent_ch = (uint8_t)(ch - 1);
        kg.sent_fx = (int8_t)fx;
        kg_cc(kg.sent_ch, KG_CC[fx], 127);
    }
    if (ch > 0)                                       /* the macros, when they change */
        for (i = 0; i < 3u; i++) {
            int16_t v = (int16_t)(kg_pct((int32_t)(G_KCHAOS + i)) * 127u / 100u);
            if (v != kg.sent_mac[i]) {
                kg.sent_mac[i] = v;
                kg_cc((uint32_t)ch - 1u, KG_CC_CHAOS + i, (uint32_t)v);
            }
        }
    else
        kg.sent_mac[0] = kg.sent_mac[1] = kg.sent_mac[2] = -1;
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
