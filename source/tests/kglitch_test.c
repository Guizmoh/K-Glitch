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
#include <stdlib.h>
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
    song.g[G_KBANK] = 0, song.g[G_KINT] = 50, song.g[G_KHOLD] = 0, song.g[G_KLFO] = 0, song.g[G_KCMODE] = 0, song.g[G_KEVERY] = 0;
    for (cc_n = 0; cc_n < KD_SLOTS; cc_n++) kd_slot[cc_n].cur = -1, kd_slot[cc_n].g = 0, kd_wantv[cc_n] = -1, kd_sintv[cc_n] = 0;
    kg_hold_clear();
    song.g[G_KMULTI] = 0;
    kg.sent_cc = 0;
    kl.st = KLS_EMPTY, song.g[G_KLMODE] = 0;
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
        kd_process(l, r, CTL);                        /* (as fx.c mix_block: the DRUM DESTROYER bank after) */
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
        else assert(((cc >= KG_CC_CHAOS && cc <= KG_CC_MOVE) || cc == KG_CC_INT) && n++ < 4u);
    }
    assert(k == 4u && n == 4u);                       /* (two bars: two STUTTERs, on and off; 3 macros, INTENSITY) */
    printf("kglitch: OUT MPC: the FM-1 stays dry, CC %u 127/0 on ch 16, macros CC 85-87          ok\n", KG_CC[PX_STUT]);

    /* 4. leaving MPC while the effect is up lets it go; a new channel too */
    reset();
    kg.seq[4] = PX_STUT;
    song.g[G_KTGT] = KG_OUT_BOTH;
    run(4u * S16 + S16 / 2u, -1, 0, 0, ol, or_);
    assert(!same(ol, dl, 4u * S16, 4u * S16 + S16 / 2u) && "BOTH: the mix changes too");
    assert(kg.sent_cc >> PX_STUT & 1u);
    song.g[G_KTGT] = KG_OUT_FM1;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(!kg.sent_cc && (cc_log[cc_n - 1u] & 0xFFFFu) == (uint32_t)(KG_CC[PX_STUT] << 8));
    song.g[G_KTGT] = KG_OUT_MPC;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert((kg.sent_cc >> PX_STUT & 1u) && kg.sent_ch == 15u);
    song.g[G_KCCCH] = 3;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(kg.sent_ch == 2u && cc_log[cc_n - 2u] == (0xBFu << 16 | (uint32_t)KG_CC[PX_STUT] << 8) &&
           (cc_log[cc_n - 1u] >> 16 == 0xB2u || (cc_log[cc_n - 1u] >> 8 & 127u) >= KG_CC_CHAOS));
    song.playing = 0;
    { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); midi_drain(); }
    assert(!kg.sent_cc && "stopped: nothing left up");
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
    /* 7. the DRUM DESTROYER bank: each effect at INTENSITY 0, 50, 100, playing and stopped, held from 0.5 s to 2.5 s
     * (longer than the ring): it changes the mix, stays bounded (run), and 0.1 s after its release the mix is the dry
     * one again to the sample */
    for (k = 0; k < DD_N; k++) {
        uint32_t in, pl;
        for (pl = 0; pl < 2u; pl++)
            for (in = 0; in <= 100u; in += 50u) {
                reset();
                song.playing = (uint8_t)pl;
                song.g[G_KINT] = (int16_t)in;
                run(NS, (int32_t)(PUNCH_NFX + k), FS / 2u, FS * 5u / 2u, ol, or_);
                if (!(k == DD_GATE && in == 0u) && !(k == DD_BSKIP && in == 0u))
                    assert(!same(ol, dl, FS / 2u, FS * 5u / 2u) && "the effect changes the mix");
                assert(same(ol, dl, FS * 5u / 2u + FS / 10u, NS / CTL * CTL) && "dry again after its release");
                assert(kd.cur < 0);
            }
        printf("kglitch: DRUM DESTROYER %-13s INTENSITY 0 50 100, playing and stopped: wet, bounded, dry after   ok\n", DD_NAME[k]);
    }
    /* a held LOOP DESTROY outlives the ring (0.74 s): 2 s later it still repeats its fragment, period L */
    reset();
    song.g[G_KINT] = 40;                                /* (no random direction: a plain loop) */
    run(NS, (int32_t)(PUNCH_NFX + DD_LOOP), FS / 4u, NS, ol, or_);
    {
        uint32_t L = kd.L, d = 0;
        for (i = FS * 3u; i < FS * 3u + 2000u; i++)
            d += (uint32_t)abs(ol[i] - ol[i - L]);
        assert(kd.cur == DD_LOOP && L > 1000u && d < 2000u * 64u && "the fragment is kept");
    }
    printf("kglitch: LOOP DESTROY held 3.75 s: the fragment kept from the ring's writer                ok\n");
    /* BANK DESTROY: AUTO and MUTATE pick the DRUM DESTROYER effects */
    reset();
    song.g[G_KBANK] = 1, song.g[G_KSEQ] = 0, song.g[G_KAUTO] = 1, song.g[G_KDENS] = 100, song.g[G_KCHAOS] = 100;
    run(NS, -1, 0, 0, ol, or_);
    for (i = 0; i < 32u; i++) assert(now16[i] >= (int8_t)PUNCH_NFX && now16[i] < (int8_t)KG_NFX);
    kg_mutate(7);
    for (i = 0; i < 16u; i++) assert(kg.seq[i] < 0 || kg.seq[i] >= (int8_t)PUNCH_NFX);
    printf("kglitch: BANK DESTROY: AUTO and MUTATE pick the DRUM DESTROYER bank                       ok\n");
    /* 8. DRUMS: every style writes two bars (hits on kicks and snares), its tempo, a kit; it is the ORIGINAL */
    for (k = 0; k < KS_N; k++) {
        uint32_t hits = 0, kick = 0;
        reset();
        song.g[G_KVAR] = 0;
        kr_generate(k);
        for (i = 0; i < 32u; i++) {
            hits += (uint32_t)__builtin_popcount(TDRUM->dstep[i].on[0] | TDRUM->dstep[i].on[1] << 8);
            kick += dstep_has(&TDRUM->dstep[i], 0);
        }
        assert(hits > 16u && kick >= 3u && TDRUM->p[P_SLEN] == 32 && song.g[G_BPM] == KG_STYLE[k].bpm && kr.has && !kr.mutated);
        assert(TDRUM->p[P_E0] == (int16_t)(DRUM_SAMPLED + KG_STYLE[k].kit));
    }
    printf("kglitch: DRUMS: the %u styles write two bars, their tempo and kit, the ORIGINAL           ok\n", KS_N);
    /* 9. CHOP: CHAOS 0 DENSITY 0 changes nothing; at 100 it does; always from the ORIGINAL; RESTORE is exact; a change
     * by hand becomes the ORIGINAL */
    {
        uint32_t sig_o, m1, m2;
        reset();
        song.g[G_KVAR] = 0;
        kr_generate(KS_JUNGLE);
        sig_o = kr_sig();
        song.g[G_KCHAOS] = 0, song.g[G_KDENS] = 0;
        kr_mutate(1);
        assert(kr_sig() == sig_o && kr.changed == 0u);
        song.g[G_KCHAOS] = 100, song.g[G_KDENS] = 100;
        kr_mutate(1);
        m1 = kr_sig();
        assert(m1 != sig_o && kr.changed > 0u && kr.mutated);
        for (i = 0; i < 50u; i++) kr_mutate(1);
        for (i = 0; i < 32u; i++)                       /* (never further than one mutation from the ORIGINAL: the */
            assert(TDRUM->micro[i] >= MICRO_MIN && TDRUM->micro[i] <= MICRO_MAX);   /* micro timing bounded) */
        kr_restore();
        assert(kr_sig() == sig_o && !kr.mutated);
        kr_mutate(0);
        m2 = kr_sig();
        dstep_set(&TDRUM->dstep[1], 9, LV_NORM, 0);    /* by hand */
        kr_mutate(0);
        kr_restore();
        assert(dstep_has(&TDRUM->dstep[1], 9) && kr_sig() != sig_o && m2 != sig_o);   /* the hand's pattern is the ORIGINAL */
    }
    printf("kglitch: CHOP: no change at 0, from the ORIGINAL, RESTORE exact, a hand edit kept        ok\n");
    /* 10. CHOP EVERY 1 BAR and CHAOS mode mutate on the bar while PLAY runs (through mix_block, the real path) */
    {
        static int32_t o[CTL * 2u];
        uint32_t b, sig_o, fxs = 0;
        reset();
        song.g[G_KVAR] = 0;
        kr_generate(KS_DNB);
        sig_o = kr_sig();
        song.g[G_KEVERY] = 1;
        song.playing = 0;
        transport_req = 1;
        for (b = 0; b < FS * 3u / CTL; b++) mix_block(o, CTL);
        assert(kr.mutated && kr_sig() != sig_o && "EVERY 1 BAR");
        transport_req = 2; mix_block(o, CTL);
        kr_restore();
        song.g[G_KEVERY] = 0, song.g[G_KCMODE] = 1, song.g[G_KCHAOS] = 100;
        transport_req = 1;
        for (b = 0; b < FS * 3u / CTL; b++) {
            mix_block(o, CTL);
            fxs += kg.now >= 0;
            for (i = 0; i < CTL * 2u; i++) assert(o[i] >= -32768 && o[i] <= 32767);
        }
        assert(kr.mutated && fxs > 100u && "CHAOS mode: mutations and AUTO");
        transport_req = 2; mix_block(o, CTL);
        song.g[G_KCMODE] = 0;
    }
    printf("kglitch: CHOP EVERY and CHAOS mode mutate on the bar (mix_block), output bounded        ok\n");
    /* 11. the LFO: off 0; on, within MOVEMENT / 2; INTENSITY per effect */
    {
        int32_t lo = 100, hi = -100;
        reset();
        song.g[G_KLFO] = 1, song.g[G_KMOVE] = 80;
        run(NS, -1, 0, 0, ol, or_);
        for (i = 0; i < 400u; i++) {
            int32_t l[CTL] = {0}, r[CTL] = {0};
            punch_process(l, r, CTL);
            clk_pos += CTL * 120u * 40u; if (clk_pos >= BEAT_U) clk_pos -= BEAT_U, clk_beat++;
            if (kd_lfo < lo) lo = kd_lfo;
            if (kd_lfo > hi) hi = kd_lfo;
        }
        assert(lo >= -40 && hi <= 40 && hi - lo > 40 && "the LFO swings within MOVEMENT / 2");
        song.g[G_KLFO] = 0;
        { int32_t l[CTL] = {0}, r[CTL] = {0}; punch_process(l, r, CTL); }
        assert(kd_lfo == 0);
        kd_int[DD_FUZZ] = 90; kd.cur = DD_FUZZ;
        assert(dd_int() == 90u);
        kd.cur = -1; kd_int[DD_FUZZ] = 50;
    }
    printf("kglitch: LFO within MOVEMENT / 2 (off: 0); INTENSITY per effect                         ok\n");
    /* 12. DICE: a recipe of 2..5 named transformations, from the ORIGINAL; RESTORE exact */
    {
        uint32_t sig_o, d, plus;
        reset();
        song.g[G_KVAR] = 0;
        kr_generate(KS_BREAKCORE);
        sig_o = kr_sig();
        for (d = 0; d < 30u; d++) {
            kr_dice(1);
            for (i = plus = 0; kr.recipe[i]; i++) plus += kr.recipe[i] == '+';
            assert(kr.recipe[0] && plus >= 1u && plus <= 4u && kr.mutated);
        }
        kr_restore();
        assert(kr_sig() == sig_o);
    }
    printf("kglitch: CHOP DICE: 2 to 5 named transformations, from the ORIGINAL, RESTORE exact        ok\n");
    /* 13. automation: REC + a pad writes its 16ths with its INTENSITY; the lane plays it back with that INTENSITY
     * (not the effect's own, changed since); CLEAR held (kg_erase) erases the steps passing, and nothing plays meanwhile */
    {
        uint32_t n = 0;
        reset();
        song.g[G_KSEQ] = 2;                             /* 32 steps */
        kd_int[DD_RING] = 30;
        song.rec = 1;
        run(NS / 2u, (int32_t)(PUNCH_NFX + DD_RING), S16 * 4u, S16 * 8u, ol, or_);
        song.rec = 0;
        for (i = 0; i < 32u; i++)
            if (kg.seq[i] == (int8_t)(PUNCH_NFX + DD_RING)) { assert(kg.sint[i] == 31u); n++; }
        assert(n >= 4u && n <= 5u && "a beat of 16ths recorded");
        kd_int[DD_RING] = 90;                           /* (changed since: the lane keeps 30) */
        clk_beat = 8, clk_pos = 0;                      /* (two bars on: step 4 again) */
        kg.last16 = 0xFFFFFFFFu;
        run(S16 * 6u, -1, 0, 0, ol, or_);
        assert(now16[4] == (int8_t)(PUNCH_NFX + DD_RING) && kg.src == 2u && dd_int() == 30u);
        kd_int[DD_RING] = 50;
        clk_beat = 8, clk_pos = 0, kg.last16 = 0xFFFFFFFFu;
        kg_erase = 1;
        run(S16 * 10u, -1, 0, 0, ol, or_);
        kg_erase = 0;
        for (i = 0; i < 10u; i++) assert(kg.seq[i] < 0 && now16[i] < 0);
    }
    printf("kglitch: automation: REC writes the pad's 16ths and INTENSITY, plays them back, CLEAR erases ok\n");
    /* 14. probability: a step at 0 % never plays, at 100 % always, at 50 % about half the time */
    {
        uint32_t bars, hits = 0;
        reset();
        kg.seq[4] = PX_STUT, kg.prob[4] = 0;
        kg.seq[8] = PX_REV, kg.prob[8] = 50;
        for (bars = 0; bars < 16u; bars++) {
            clk_beat = bars * 4u, clk_pos = 0, kg.last16 = 0xFFFFFFFFu;
            run(S16 * 10u, -1, 0, 0, ol, or_);
            assert(now16[4] != PX_STUT && now16[0] == -1);
            hits += now16[8] == PX_REV;
        }
        assert(hits >= 3u && hits <= 13u);
    }
    printf("kglitch: SEQ probability: 0 %% never, 50 %% about half the bars                            ok\n");
    /* 15. MULTI: three pads at once: three slots, the mix changed more than by one; the CCs of all three; SOLO: one */
    {
        uint32_t k3;
        reset();
        song.g[G_KMULTI] = 1, song.g[G_KTGT] = KG_OUT_BOTH;
        kg_hold_add((int32_t)(PUNCH_NFX + DD_GATE));
        kg_hold_add((int32_t)(PUNCH_NFX + DD_RING));
        kg_hold_add(PX_STUT);
        run(NS / 2u, -1, 0, 0, ol, or_);
        for (k3 = n = 0; k3 < KD_SLOTS; k3++) n += kd_slot[k3].cur >= 0;
        assert(n == 2u && kg.nfx == 3u && punch.cur == PX_STUT);
        assert((kg.sent_cc >> PX_STUT & 1u) && (kg.sent_cc >> DD_GATE & 1u) && (kg.sent_cc >> DD_RING & 1u));
        assert(!same(ol, dl, S16, NS / 2u / CTL * CTL));
        kg_hold_clear();
        song.g[G_KMULTI] = 0;
        run(S16 * 4u, (int32_t)(PUNCH_NFX + DD_FUZZ), 0, S16 * 4u, ol, or_);
        assert(kg.nfx == 1u);
    }
    printf("kglitch: MULTI: three at once (two DRUM DESTROYER slots + SLOOP's), their CCs; SOLO one    ok\n");
    /* 16. LOOP: armed, it records a bar from the next downbeat into the delay's line, then plays it back locked to the
     * bar (the tracks muted: what plays is the loop); SLICE reversed / retriggered differs from LOOP; GRAIN sounds;
     * the delay is silent meanwhile; OFF gives the line back */
    {
        static int32_t o[CTL * 2u], a[FS], bb[FS];
        uint32_t b, j;
        double e = 0;
        reset();
        song.g[G_KVAR] = 0;
        kr_generate(KS_JUNGLE);
        song.g[G_KLBARS] = 0;
        kl_arm();
        transport_req = 1;
        for (b = 0; b < FS * 4u / CTL && kl.st != KLS_HAS; b++) mix_block(o, CTL);
        assert(kl.st == KLS_HAS && kl.bars == 1u && kl.len == kl_bar_samples() / 2u && song.g[G_KLMODE] == KL_LOOP);
        assert(kl_line());
        for (j = 0; j < 64u; j++) e += (double)kl_buf[j * 512u] * kl_buf[j * 512u];
        assert(e > 0 && "something recorded");
        for (b = 0; b < FS / CTL; b++) {               /* a second of the loop */
            mix_block(o, CTL);
            for (j = 0; j < CTL; j++) a[b * CTL + j] = o[2u * j];
        }
        for (j = 0, e = 0; j < FS / CTL * CTL; j++) e += (double)a[j] * a[j];
        assert(e > 1e9 && "the loop plays");
        song.g[G_KLMODE] = KL_SLICE;
        for (j = 0; j < KL_NS; j++) kl.rev[j] = 1, kl.ret[j] = 3;
        for (b = 0; b < FS / CTL; b++) {
            mix_block(o, CTL);
            for (j = 0; j < CTL; j++) bb[b * CTL + j] = o[2u * j];
        }
        assert(!same(a, bb, 0, FS / CTL * CTL) && "SLICE: reversed, retriggered");
        song.g[G_KLMODE] = KL_GRAIN, song.g[G_KGSIZE] = 30, song.g[G_KGDENS] = 80;
        for (b = 0, e = 0; b < FS / CTL; b++) {
            mix_block(o, CTL);
            for (j = 0; j < CTL * 2u; j++) { e += (double)o[j] * o[j]; assert(o[j] >= -32768 && o[j] <= 32767); }
        }
        assert(e > 1e9 && "GRAIN sounds");
        kl_free();
        assert(!kl_line());
        transport_req = 2; mix_block(o, CTL);
    }
    printf("kglitch: LOOP: a bar recorded on the downbeat, played locked, SLICE, GRAIN, the line back  ok\n");
    /* 17. GLITCH (eng_glitch.c): every preset, notes across the keyboard: it sounds, stays bounded, ends */
    {
        static int32_t o[CTL * 2u];
        uint32_t pi, b, note;
        assert(ENGINES[ENGI_GLITCH] == &ENG_GLITCH);
        for (pi = 0; pi < ENG_GLITCH.npresets; pi++)
            for (note = 36; note <= 96; note += 30) {
                double e = 0;
                reset();
                song.playing = 0;
                host_preset(&trk[0], ENGI_GLITCH, pi);
                trk_note_on(&trk[0], note, 110);
                for (b = 0; b < FS / 2u / CTL; b++) {
                    mix_block(o, CTL);
                    for (i = 0; i < CTL * 2u; i++) { e += (double)o[i] * o[i]; assert(o[i] >= -32768 && o[i] <= 32767); }
                }
                trk_note_off(&trk[0], note);
                for (b = 0; b < FS * 2u / CTL; b++) mix_block(o, CTL);
                assert(e > 1e6 && "it sounds");
            }
        printf("kglitch: GLITCH: the %u presets sound across the keyboard, bounded                     ok\n", ENG_GLITCH.npresets);
    }
    /* 18. the state kept in flash (kg_pack / kg_unpack): back exactly; a damaged record changes nothing; the loop's
     * MODE is not brought back (its sound is RAM only) */
    {
        static kg_blob_t a, c;
        uint32_t k;
        reset();
        for (k = 0; k < KG_STEPS; k++) kg.seq[k] = (int8_t)(k % KG_NFX), kg.seq2[k] = -1, kg.sint[k] = (uint8_t)k, kg.prob[k] = (uint8_t)(k % 101u);
        kd_int[3] = 77, kl.map[2] = 9, kl.rev[5] = 1, kl.ret[7] = 8;
        song.g[G_KCHAOS] = 66, song.g[G_KSEQ] = 3, song.g[G_KMULTI] = 1, song.g[G_KLMODE] = KL_GRAIN;
        kg_pack(&a);
        reset();
        kd_int[3] = 50, kl.map[2] = 2, kl.rev[5] = 0, kl.ret[7] = 1;
        assert(kg_unpack(&a) == 0);
        kg_pack(&c);
        assert(kg.seq[5] == 5 && kg.prob[40] == 40 && kd_int[3] == 77 && kl.map[2] == 9 && kl.ret[7] == 8);
        assert(song.g[G_KCHAOS] == 66 && song.g[G_KSEQ] == 3 && song.g[G_KMULTI] == 1 && song.g[G_KLMODE] == 0);
        a.seq[3] ^= 1;                                  /* damaged */
        kd_int[3] = 12;
        assert(kg_unpack(&a) == -1 && kd_int[3] == 12);
        kd_int[3] = 50, kl.map[2] = 2, kl.rev[5] = 0, kl.ret[7] = 1;
    }
    printf("kglitch: the state for flash: back exactly, a damaged record refused                     ok\n");
    printf("kglitch_test: passed\n");
    return 0;
}
