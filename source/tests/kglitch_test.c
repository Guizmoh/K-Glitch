/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH (firmware/src/kglitch.c) over SLOOP's punch-in FX:
 *   the effect sequencer plays its step's effect on its 16th, and only then;
 *   a key (punch.req) wins over the sequencer, the sequencer over AUTO;
 *   OUT MPC: the FM-1's mix stays dry to the sample, and each effect goes out as one CC 127 / 0 on the MPC channel,
 *   with the macros as CC 85..87; OUT BOTH: the mix changes and the CCs go out; FM-1: no MIDI at all;
 *   leaving MPC or changing the channel lets go of the CC still up (no stuck effect on the MPC);
 *   AUTO: DENSITY 0 nothing, 100 a gesture on every 16th, only effects of the CHAOS pool, bounded output;
 *   MUTATE: DENSITY 0 an empty sequence, CHAOS <= 70 leaves the beats dry, the pool follows CHAOS.
 * argv[1]: a WAV demo (a beat through AUTO at CHAOS 100). */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#define BEAT (FS / 2u)                                /* 120 BPM */
#define S16 (BEAT / 4u)
#define NS (FS * 4u)
static int32_t ol[NS], or_[NS], dl[NS], dr[NS];
static uint32_t cc_n, cc_log[256];                    /* (status << 16 | cc << 8 | value) */

static int32_t gen(uint32_t t, int ch)                /* a deterministic "beat": a decaying tone each 1/8, noise */
{
    static uint32_t seed = 1;
    uint32_t q = t % (BEAT / 2u);
    int32_t tone = (sine_i(t * 10712093u) * (int32_t)(BEAT / 2u - q) / (int32_t)(BEAT / 2u)) >> 1;
    if (t == 0) seed = 1;
    seed = seed * 1664525u + 1013904223u;
    return (tone + (ch ? -1 : 1) * ((int32_t)(seed >> 20) - 2048)) * 3;
}
static void midi_drain(void)
{
    while (mo_r != mo_w) {
        uint32_t p = midi_out_q[mo_r % MQ];
        mo_r++;
        if (cc_n < 256u) cc_log[cc_n++] = ((p >> 8) & 0xFFu) << 16 | ((p >> 16) & 0x7Fu) << 8 | (p >> 24 & 0x7Fu);
    }
}
static void reset(void)
{
    host_tracks_init();
    song.g[G_BPM] = 120;
    punch.req = -1, punch.cur = -1, punch.g = 0, punch.hold = 0;
    kg_clear();
    kg.last16 = 0xFFFFFFFFu, kg.auto_fx = -1, kg.auto_left = 0, kg.now = -1;
    kg.sent_mac[0] = kg.sent_mac[1] = kg.sent_mac[2] = -1;
    song.g[G_KCHAOS] = 40, song.g[G_KDENS] = 50, song.g[G_KMOVE] = 50;
    song.g[G_KTGT] = KG_OUT_FM1, song.g[G_KSEQ] = 1, song.g[G_KAUTO] = 0, song.g[G_KCCCH] = 16;
    clk_beat = 0, clk_pos = 0;
    song.playing = 1;
    usb.config = 1;
    mo_r = mo_w = 0;
    cc_n = 0;
}
/* ns samples through punch_process; key: the punch.req held from `on` to `off` (-1: none). now[] each 16th */
static int8_t now16[64];
static void run(uint32_t ns, int32_t key, uint32_t on, uint32_t off, int32_t *L, int32_t *R)
{
    uint32_t b, i;
    for (b = 0; b < ns / CTL; b++) {
        int32_t l[CTL], r[CTL];
        uint32_t t0 = b * CTL;
        for (i = 0; i < CTL; i++) l[i] = gen(t0 + i, 0), r[i] = gen(t0 + i, 1);
        punch.req = (int8_t)(key >= 0 && t0 >= on && t0 < off ? key : -1);
        punch_process(l, r, CTL);
        if (song.playing && (t0 % S16) >= S16 / 2u && (t0 % S16) < S16 / 2u + CTL && t0 / S16 < 64u)   /* (mid-16th) */
            now16[t0 / S16] = kg.now;
        clk_pos += CTL * (uint32_t)song.g[G_BPM];
        if (clk_pos >= BEAT_U) clk_pos -= BEAT_U, clk_beat++;
        for (i = 0; i < CTL; i++) {
            assert(l[i] > -(1 << 22) && l[i] < (1 << 22) && r[i] > -(1 << 22) && r[i] < (1 << 22));
            L[t0 + i] = l[i], R[t0 + i] = r[i];
        }
        midi_drain();
    }
    punch.req = -1;
}
static int same(const int32_t *a, const int32_t *b, uint32_t s, uint32_t e)
{
    uint32_t i;
    for (i = s; i < e; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}
static int in_pool(int32_t fx, uint32_t chaos)
{
    uint32_t i, n = 4u + chaos * (KG_NPOOL - 4u) / 100u;
    for (i = 0; i < n; i++)
        if (KG_POOL[i] == fx) return 1;
    return 0;
}
static void wav(const char *path, const int32_t *l, const int32_t *r, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    uint32_t i;
    if (!f) return;
    wav_hdr(f, n);
    for (i = 0; i < n; i++) {
        int16_t s[2] = {(int16_t)clamp(l[i] >> 3, -32768, 32767), (int16_t)clamp(r[i] >> 3, -32768, 32767)};
        fwrite(s, 2, 2, f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    uint32_t i, k, n;
    /* the dry reference: nothing asked */
    reset();
    run(NS, -1, 0, 0, dl, dr);
    assert(cc_n == 0u && "FM-1: no MIDI");

    /* 1. the sequencer: STUTTER on step 5 (the 16th 4), only there */
    reset();
    kg.seq[4] = PX_STUT;
    run(NS, -1, 0, 0, ol, or_);
    assert(now16[4] == PX_STUT && now16[3] == -1 && now16[5] == -1 && now16[20] == PX_STUT);
    assert(same(ol, dl, 0, 4u * S16) && "dry before its step");
    assert(!same(ol, dl, 4u * S16, 6u * S16) && "the step changes the mix");
    assert(same(ol, dl, 7u * S16, 20u * S16 - 1u) && "dry again after it (and its fade)");
    assert(cc_n == 0u);
    printf("kglitch: the effect sequencer plays its step's effect, dry around it                ok\n");

    /* 2. a key wins over the sequencer, the sequencer over AUTO */
    reset();
    kg.seq[4] = PX_STUT;
    song.g[G_KAUTO] = 1, song.g[G_KDENS] = 100;
    run(NS, PX_REV, 4u * S16, 5u * S16, ol, or_);
    assert(now16[4] == PX_REV && now16[20] == PX_STUT && kg.src);
    for (i = 0; i < 32u; i++) assert(now16[i] >= 0);   /* (2 bars) */   /* (DENSITY 100: something on every 16th) */
    printf("kglitch: priority key > sequencer > AUTO                                             ok\n");

    /* 3. OUT MPC: the mix dry to the sample, CC 127 then 0 for STUTTER on channel 16, the macros once */
    reset();
    kg.seq[4] = PX_STUT;
    song.g[G_KTGT] = KG_OUT_MPC;
    run(NS, -1, 0, 0, ol, or_);
    assert(same(ol, dl, 0, NS / CTL * CTL) && same(or_, dr, 0, NS / CTL * CTL));
    for (i = k = n = 0; i < cc_n; i++) {
        uint32_t st = cc_log[i] >> 16, cc = cc_log[i] >> 8 & 127u, v = cc_log[i] & 127u;
        assert(st == 0xBFu);
        if (cc == KG_CC[PX_STUT]) assert(v == (k++ & 1u ? 0u : 127u));
        else assert(cc >= KG_CC_CHAOS && cc <= KG_CC_MOVE && n++ < 3u);
    }
    assert(k == 4u && n == 3u);                       /* (two bars: two STUTTERs, on and off) */
    printf("kglitch: OUT MPC: the FM-1 stays dry, CC %u 127/0 on ch 16, macros CC 85-87          ok\n", KG_CC[PX_STUT]);

    /* 4. leaving MPC while the effect is up lets it go; a new channel too */
    reset();
    kg.seq[4] = PX_STUT;
    song.g[G_KTGT] = KG_OUT_BOTH;
    run(4u * S16 + S16 / 2u, -1, 0, 0, ol, or_);
    assert(!same(ol, dl, 4u * S16, 4u * S16 + S16 / 2u) && "BOTH: the mix changes too");
    assert(kg.sent_fx == PX_STUT);
    song.g[G_KTGT] = KG_OUT_FM1;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(kg.sent_fx < 0 && (cc_log[cc_n - 1u] & 0xFFFFu) == (uint32_t)(KG_CC[PX_STUT] << 8));
    song.g[G_KTGT] = KG_OUT_MPC;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(kg.sent_fx == PX_STUT && kg.sent_ch == 15u);
    song.g[G_KCCCH] = 3;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(kg.sent_ch == 2u && cc_log[cc_n - 2u] == (0xBFu << 16 | (uint32_t)KG_CC[PX_STUT] << 8) &&
           (cc_log[cc_n - 1u] >> 16 == 0xB2u || (cc_log[cc_n - 1u] >> 8 & 127u) >= KG_CC_CHAOS));
    song.playing = 0;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(kg.sent_fx < 0 && "stopped: nothing left up");
    printf("kglitch: OUT BOTH changes the mix; leaving MPC, a new channel, STOP: no stuck CC       ok\n");

    /* 5. AUTO: DENSITY 0 nothing; 100: every 16th, from the CHAOS pool */
    for (k = 0; k <= 100u; k += 50u) {
        reset();
        song.g[G_KSEQ] = 0, song.g[G_KAUTO] = 1, song.g[G_KDENS] = 0, song.g[G_KCHAOS] = (int16_t)k;
        run(NS / 2u, -1, 0, 0, ol, or_);
        for (i = 0; i < 16u; i++) assert(now16[i] == -1);
        assert(same(ol, dl, 0, NS / 2u / CTL * CTL));
        song.g[G_KDENS] = 100;
        run(NS, -1, 0, 0, ol, or_);
        for (i = 0; i < 32u; i++) assert(now16[i] >= 0 && in_pool(now16[i], k));
    }
    if (argc > 1) wav(argv[1], ol, or_, NS);
    printf("kglitch: AUTO: DENSITY 0 dry, 100 a gesture every 16th, from the CHAOS pool, bounded ok\n");

    /* 6. MUTATE */
    song.g[G_KDENS] = 0;
    kg_mutate(1);
    for (i = 0; i < 16u; i++) assert(kg.seq[i] == -1);
    song.g[G_KDENS] = 100, song.g[G_KCHAOS] = 0;
    kg_mutate(2);
    for (i = 0; i < 16u; i++) assert((i & 3u) == 0u ? kg.seq[i] == -1 : in_pool(kg.seq[i], 0));
    song.g[G_KCHAOS] = 100;
    for (k = n = 0; k < 20u; k++) {
        kg_mutate(k + 3u);
        for (i = 0; i < 16u; i++) {
            assert(kg.seq[i] >= 0 && kg.seq[i] < (int8_t)PUNCH_NFX);
            n += !in_pool(kg.seq[i], 0);
        }
    }
    assert(n > 100u && "CHAOS 100: mostly past the gentle four");
    printf("kglitch: MUTATE: DENSITY, CHAOS (beats dry up to 70 %%), the pool                     ok\n");
    printf("kglitch_test: passed\n");
    return 0;
}
