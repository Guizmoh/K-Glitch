/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH engine tests: the break engine, the effects and the ADPCM decoder on a computer, with plain gcc.
 * Writes WAV files of a few renders to OUTDIR to listen to.   tests/engine/run.sh */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adpcm.h"
#include "brk.h"
#include "brk_fx.h"
#include "brk_synth.h"
#include "kick.h"
#include "ovl.h"
#include "perc.h"

#define RATE 22050u
#define BPM 136.9f
#define BLOCK 256u

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: ", __FILE__, __LINE__); \
    printf(__VA_ARGS__); printf("\n"); } } while (0)

static int16_t pcm[200000];
static brk_break_t brk;
static const char *outdir = ".";

static void fresh(uint32_t bars)
{
    brk_init();
    brk_synth_break(pcm, RATE, BPM, bars, &brk);
    brk.split = 0u;
    brk_set_break(&brk);
    brk_tempo(BPM);
    brk_pitch(0.0f);
    brk_swing(0.5f);
    brk_snap(1.0f);                      /* (keys at the next step: a slice is a step on these 1-bar breaks) */
    brk_seed(1u);
    {
        uint32_t p;
        for (p = 0; p < BRK_NPADS; p++)
            brk_pad((int)p, 0);
    }
}

/* renders n frames in blocks; slices[]: the slice sounding at the start of each step seen (may be 0) */
static float render(float *out, uint32_t n, int32_t *steps_seen, int32_t *slices, uint32_t max)
{
    uint32_t i = 0, k = 0;
    int32_t last_step = -2;
    float peak = 0.0f;
    while (i < n) {
        uint32_t m = n - i < BLOCK ? n - i : BLOCK, j;
        brk_status_t st;
        brk_render(out + i, m);
        brk_status(&st);
        if (slices && st.step != last_step && k < max) {
            slices[k++] = st.slice;
            last_step = st.step;
        }
        for (j = 0; j < m; j++)
            if (fabsf(out[i + j]) > peak)
                peak = fabsf(out[i + j]);
        i += m;
    }
    if (steps_seen)
        *steps_seen = (int32_t)k;
    return peak;
}

static void wav(const char *name, const float *l, const float *r, uint32_t n)
{
    char path[512];
    FILE *f;
    uint32_t i, bytes = n * 4u, v;
    snprintf(path, sizeof path, "%s/%s", outdir, name);
    if (!(f = fopen(path, "wb")))
        return;
    fwrite("RIFF", 1, 4, f);
    v = 36u + bytes; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v = 16u; fwrite(&v, 4, 1, f);
    { uint16_t a = 1, c = 2; fwrite(&a, 2, 1, f); fwrite(&c, 2, 1, f); }
    v = BRK_SR; fwrite(&v, 4, 1, f);
    v = BRK_SR * 4u; fwrite(&v, 4, 1, f);
    { uint16_t a = 4, b = 16; fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f); }
    fwrite("data", 1, 4, f);
    fwrite(&bytes, 4, 1, f);
    for (i = 0; i < n; i++) {
        int16_t s[2] = {(int16_t)(l[i] * 32000.0f), (int16_t)((r ? r[i] : l[i]) * 32000.0f)};
        fwrite(s, 2, 2, f);
    }
    fclose(f);
}

static float buf[BRK_SR * 30u], bl[BRK_SR * 30u], br[BRK_SR * 30u];

static void test_straight(void)
{
    int32_t seen, sl[64], i, ok = 1;
    uint32_t bar = (uint32_t)(BRK_SR * 60.0f / BPM * 4.0f);
    fresh(1);
    brk_play(1);
    render(buf, bar, &seen, sl, 64);
    /* each step is about 19 blocks long, so every step is seen and the loop plays its slices in order */
    CHECK(seen >= 16, "steps seen %d", seen);
    for (i = 0; i < 16 && i < seen; i++)
        if (sl[i] != i)
            ok = 0;
    CHECK(ok, "straight loop plays slices 0..15 in order");
    wav("straight.wav", buf, 0, bar);
}

static void test_tempo_accuracy(void)
{
    /* 64 bars at 174 BPM: the 1024th step starts within 1 sample of where it should */
    uint32_t n = 0, steps = 0, target = 64u * 16u;
    double expect = (double)BRK_SR * 60.0 / (174.0 * 4.0) * target;
    int32_t last = -1;
    brk_status_t st;
    fresh(1);
    brk_tempo(174.0f);
    brk_play(1);
    while (steps < target + 1u) {        /* one frame at a time: the exact sample a step starts */
        brk_render(buf, 1);
        brk_status(&st);
        if (st.step != last) {
            last = st.step;
            if (steps == target)
                break;
            steps++;
        }
        n++;
    }
    CHECK(fabs((double)n - expect) <= 1.0, "step %u at sample %u, expected %.1f", target, n, expect);
}

static void test_free_play(void)
{
    float peak;
    fresh(1);
    brk_play(0);
    render(buf, BLOCK, 0, 0, 0);
    CHECK(render(buf, BLOCK, 0, 0, 0) == 0.0f, "stopped and untouched: silence");
    brk_key(4, 1);
    peak = render(buf, BLOCK, 0, 0, 0);
    CHECK(peak > 0.05f, "free play sounds in the first block (peak %.3f)", peak);
    CHECK(fabsf(buf[0]) < 0.05f, "and fades in (first sample %.3f)", buf[0]);
    brk_key(4, 0);
}

static void test_quantised_key(void)
{
    int32_t seen, sl[8];
    uint32_t half_step = brk_step_len_samples() / 2u;
    fresh(1);
    brk_play(1);
    render(buf, half_step, 0, 0, 0); /* in the middle of step 0 */
    brk_key(9, 1);
    brk_key(9, 0);                       /* a tap */
    render(buf, brk_step_len_samples() * 2u, &seen, sl, 8);
    CHECK(seen >= 2 && sl[1] == 9, "a tap plays at the next step (got %d)", seen >= 2 ? sl[1] : -9);
    CHECK(seen >= 3 && sl[2] == 2, "then the loop goes on (got %d)", seen >= 3 ? sl[2] : -9);
}

static void test_hold_loops(void)
{
    int32_t seen, sl[8], i, ok = 1;
    fresh(1);
    brk_play(1);
    brk_key(3, 1);
    render(buf, brk_step_len_samples() * 5u, &seen, sl, 8);
    for (i = 1; i < seen; i++)
        if (sl[i] != 3)
            ok = 0;
    CHECK(ok && seen >= 4, "a held key plays its slice at every step");
    brk_key(3, 0);
}

/* LOAF BREAKER (0.9.50, FLEXOR): a white key's DECAY (brk_keyset_t.start bits 4-7, START gone): each repeat of the held
 * key fades from full; without it the slice keeps its level to the end of the repeat */
static void test_key_decay(void)
{
    float e[2];
    uint32_t n = brk_step_len_samples(), t, i;
    for (t = 0; t < 2u; t++) {
        brk_keyset_t ks;
        double a = 0.0, z = 0.0;
        fresh(1);
        brk_keyset_defaults(&ks);
        ks.len = 6u;                     /* (8 slices: one long repeat) */
        ks.start = (uint8_t)(t ? 4u << 4 : 0u);   /* (DECAY 4: ~64 ms) */
        brk_keyset(3u, &ks);
        brk_play(1);
        brk_key(3, 1);
        render(buf, n * 2u, 0, 0, 0u);   /* (to the repeat's start) */
        render(buf, n * 6u, 0, 0, 0u);
        for (i = 0; i < n; i++)
            a += (double)buf[i] * buf[i];
        for (i = n * 4u; i < n * 5u; i++)
            z += (double)buf[i] * buf[i];
        e[t] = (float)(z / (a + 1e-12));
        brk_key(3, 0);
    }
    CHECK(e[1] < e[0] * 0.05f, "DECAY: a held key's repeat fades (late/early energy %.4f against %.4f without)", e[1], e[0]);
    CHECK(brk_key_decay_ms(1u) > 20.0f && brk_key_decay_ms(15u) > 1500.0f && brk_key_decay_ms(0u) == 0.0f,
          "DECAY's times: %.0f .. %.0f ms", brk_key_decay_ms(1u), brk_key_decay_ms(15u));
}

static void test_generator(void)
{
    int32_t seen, a[64], b[64], i, same = 1, differ = 0;
    brk_gen_t g = {0.0f, 0.0f, 0.0f, 0.0f, 0};
    uint32_t bars4 = brk_step_len_samples() * 64u;
    fresh(1);
    brk_gen(&g);
    brk_play(1);
    render(buf, bars4, &seen, a, 64);
    for (i = 0; i < seen; i++)
        if (a[i] != i % 16)
            same = 0;
    CHECK(same, "complexity 0 is the straight loop");
    g.complexity = 0.8f;
    g.anchor = 0.0f;
    fresh(1);
    brk_gen(&g);
    brk_seed(42u);
    brk_play(1);
    render(buf, bars4, &seen, a, 64);
    fresh(1);
    brk_gen(&g);
    brk_seed(42u);
    brk_play(1);
    render(buf, bars4, &seen, b, 64);
    for (i = 0; i < seen; i++) {
        if (a[i] != b[i])
            same = 0;
        if (a[i] != i % 16)
            differ++;
    }
    CHECK(same, "the same seed gives the same variation");
    CHECK(differ > 16, "complexity 0.8 rearranges the loop (%d of %d steps moved)", differ, seen);
    wav("generator.wav", buf, 0, bars4);
}

static void test_pads(void)
{
    int32_t seen, sl[16], i, ok = 1;
    float peak;
    /* a roll keeps the slice it caught while the sequencer counts on */
    fresh(1);
    brk_play(1);
    render(buf, brk_step_len_samples() * 3u + 10u, 0, 0, 0);
    brk_pad(PAD_ROLL16, 1);
    render(buf, brk_step_len_samples() * 6u, &seen, sl, 16);
    /* pressed during step 3, the roll is quantised: it catches step 4's slice and repeats it */
    for (i = 2; i < seen; i++)
        if (sl[i] != sl[1])
            ok = 0;
    CHECK(ok && sl[1] == 4, "ROLL catches the next step's slice and holds it (%d)", sl[1]);
    brk_pad(PAD_ROLL16, 0);
    render(buf, brk_step_len_samples() * 2u, &seen, sl, 16);
    CHECK(seen >= 2 && sl[1] == (int32_t)((3u + 6u + 1u) % 16u), "and rejoins in phase (got %d)", sl[1]);
    /* a tape stop is silent after a beat */
    fresh(1);
    brk_play(1);
    render(buf, brk_step_len_samples() * 2u, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 1);
    render(buf, brk_step_len_samples() * 4u + 64u, 0, 0, 0);
    peak = render(buf, BLOCK, 0, 0, 0);
    CHECK(peak < 0.01f, "TAPE STOP: silent after a beat (peak %.3f)", peak);
    brk_pad(PAD_TAPESTOP, 0);
    peak = render(buf, brk_step_len_samples() * 3u, 0, 0, 0);
    CHECK(peak > 0.05f, "and back on release (peak %.3f)", peak);
    /* the mangled bounce, for the ears */
    {
        uint32_t s = brk_step_len_samples(), at = 0;
        fresh(1);
        brk_play(1);
        at += 16u * s; render(buf, 16u * s, 0, 0, 0);
        brk_pad(PAD_ROLL32, 1); render(buf + at, 4u * s, 0, 0, 0); at += 4u * s; brk_pad(PAD_ROLL32, 0);
        render(buf + at, 4u * s, 0, 0, 0); at += 4u * s;
        brk_pad(PAD_REVERSE, 1); render(buf + at, 4u * s, 0, 0, 0); at += 4u * s; brk_pad(PAD_REVERSE, 0);
        render(buf + at, 4u * s, 0, 0, 0); at += 4u * s;
        brk_pad(PAD_TAPESTOP, 1); render(buf + at, 8u * s, 0, 0, 0); at += 8u * s; brk_pad(PAD_TAPESTOP, 0);
        render(buf + at, 8u * s, 0, 0, 0); at += 8u * s;
        brk_pad(PAD_HALF, 1); render(buf + at, 8u * s, 0, 0, 0); at += 8u * s; brk_pad(PAD_HALF, 0);
        brk_pad(PAD_GATE, 1); render(buf + at, 8u * s, 0, 0, 0); at += 8u * s; brk_pad(PAD_GATE, 0);
        wav("pads.wav", buf, 0, at);
    }
}

static void fx_run(const brk_fx_t *p, uint32_t n)
{
    uint32_t i;
    brk_fx_init();
    brk_fx_set(p);
    for (i = 0; i < n; i += BLOCK)
        brk_fx_process(buf + i, 0, 0, 0, 0, bl + i, br + i, n - i < BLOCK ? n - i : BLOCK, 0u);
}

static void test_fx(void)
{
    brk_fx_t p;
    uint32_t n = brk_step_len_samples() * 32u, i, ok = 1, t, silent = 0, same = 1;
    float peak, diff = 0.0f;
    static float ref[BRK_SR * 2u];
    fresh(1);
    brk_play(1);
    render(buf, n, 0, 0, 0);
    for (t = 1; t <= BRK_NFXT; t++) {    /* each effect alone in slot 1, then the default chain all on */
        brk_fx_defaults(&p);
        if (t < BRK_NFXT) {
            brk_fx_slot_defaults(&p.slot[0], (int)t);
            p.slot[0].on = 1u;
        } else
            for (i = 0; i < BRK_FX_SLOTS; i++)
                p.slot[i].on = 1u;
        fx_run(&p, n);
        for (i = 0, peak = 0.0f; i < n; i++) {
            if (!(fabsf(bl[i]) <= 1.0f) || !(fabsf(br[i]) <= 1.0f))
                ok = 0;
            if (fabsf(bl[i]) > peak)
                peak = fabsf(bl[i]);
        }
        if (peak < 0.01f)
            silent |= 1u << t;
        if (t == BRK_NFXT)
            wav("fx_chain.wav", bl, br, n);
    }
    CHECK(ok, "FX output stays finite and inside -1..1, every effect");
    CHECK(!silent, "no effect silences the break (mask %x)", silent);
    /* the order matters: DRIVE then a low-pass is not a low-pass then DRIVE */
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], FXT_DRIVE);
    brk_fx_slot_defaults(&p.slot[1], FXT_FILTER);
    p.slot[0].p[0] = 1.0f;
    p.slot[0].on = p.slot[1].on = 1u;
    fx_run(&p, BRK_SR);
    for (i = 0; i < BRK_SR; i++)
        ref[i] = bl[i];
    p.slot[0] = p.slot[1];
    brk_fx_slot_defaults(&p.slot[1], FXT_DRIVE);
    p.slot[1].p[0] = 1.0f;
    p.slot[1].on = 1u;
    fx_run(&p, BRK_SR);
    for (i = 0; i < BRK_SR; i++)
        diff += fabsf(bl[i] - ref[i]);
    CHECK(diff / (float)BRK_SR > 0.003f, "the chain's order matters (mean difference %.4f)", diff / (float)BRK_SR);
    /* FX 0: the whole chain against the dry break, all dry */
    brk_fx_defaults(&p);
    fx_run(&p, BRK_SR);
    for (i = 0; i < BRK_SR; i++)
        ref[i] = bl[i];
    for (i = 0; i < BRK_FX_SLOTS; i++)
        p.slot[i].on = 1u;
    p.wet = 0.0f;
    fx_run(&p, BRK_SR);
    for (i = 0; i < BRK_SR; i++)
        if (fabsf(bl[i] - ref[i]) > 1e-4f)
            same = 0;
    CHECK(same, "FX 0: the chain on but all dry");
}

/* an IMA ADPCM encoder for the test only (tools/prep_breaks.py has the real one) */
static const int16_t STEP[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50,
    55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494,
    544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18500, 20350, 22385, 24623, 27086, 29794, 32767};
static const int8_t ADJ[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
static void enc(const int16_t *in, uint8_t *out, uint32_t n)
{
    int32_t pred = 0, idx = 0;
    uint32_t i;
    memset(out, 0, (n + 1u) / 2u);
    for (i = 0; i < n; i++) {
        int32_t d = in[i] - pred, step = STEP[idx], code = 0, diff = step >> 3;
        if (d < 0) { code = 8; d = -d; }
        if (d >= step) { code |= 4; d -= step; diff += step; }
        if (d >= step >> 1) { code |= 2; d -= step >> 1; diff += step >> 1; }
        if (d >= step >> 2) { code |= 1; diff += step >> 2; }
        pred += (code & 8) ? -diff : diff;
        pred = pred > 32767 ? 32767 : pred < -32768 ? -32768 : pred;
        idx += ADJ[code & 7];
        idx = idx < 0 ? 0 : idx > 88 ? 88 : idx;
        out[i >> 1] |= (uint8_t)(code << ((i & 1u) * 4u));
    }
}

/* LOAF BREAKER (0.9.45): a drum's imported sample (perc_set_sample): it plays instead of the synthesis while the voice's
 * PX_RSV is PX_SAMPLE, from flash as ADPCM, as long as it is at TUNE 0.5, half as long an octave up, then quiet; its
 * fade (DECAY) shortens it; without PX_SAMPLE the synthesis again */
static void test_perc_sample(void)
{
    static int16_t src[4410];
    static uint8_t a[2205];
    static float o[44100];
    perc_t p;
    uint8_t x[PX_N] = {0};
    uint32_t i, n = 4410u, rate = 22050u, last[3];
    for (i = 0; i < n; i++)              /* 200 ms of a 300 Hz tone, fading */
        src[i] = (int16_t)(20000.0f * sinf(6.2831853f * 300.0f * (float)i / (float)rate) * (1.0f - (float)i / (float)n));
    memset(a, 0, sizeof a);
    enc(src, a, n);
    for (int t = 0; t < 3; t++) {
        perc_defaults(&p);
        p.v[PV_SNARE].tune = t == 1 ? 1.0f : 0.5f;
        p.v[PV_SNARE].decay = t == 2 ? 0.2f : 1.0f;
        p.v[PV_SNARE].tone = 0.5f;
        p.v[PV_SNARE].drive = 0.0f;
        perc_set(&p);
        x[PX_RSV] = PX_SAMPLE;
        perc_set_x(PV_SNARE, x);
        perc_set_sample(PV_SNARE, a, n, rate);
        perc_init();
        perc_hit(PV_SNARE);
        for (i = 0; i < 44100u; i += BLOCK)
            perc_render(o + i, BLOCK, 0, 0, 0u);
        last[t] = 0u;
        for (i = 0; i < 44100u; i++)
            if (fabsf(o[i]) > 0.003f)
                last[t] = i;
        if (t == 0)
            wav("perc_sample", o, o, 22050u);
    }
    CHECK(last[0] > 7600u && last[0] < 8900u, "a 200 ms sample plays 200 ms (last sound at %u)", last[0]);
    CHECK(last[1] > 3600u && last[1] < 4600u, "an octave up (TUNE right): half as long (%u)", last[1]);
    CHECK(last[2] < last[0] * 0.8f, "DECAY: a fade shortens it (%u)", last[2]);
    x[PX_RSV] = 0u;                      /* (not the sample: the synthesis, which rings on past it) */
    perc_set_x(PV_SNARE, x);
    perc_defaults(&p);
    perc_set(&p);
    perc_init();
    perc_hit(PV_SNARE);
    for (i = 0; i < 44100u; i += BLOCK)
        perc_render(o + i, BLOCK, 0, 0, 0u);
    {
        float pk = 0.0f;
        for (i = 2000u; i < 4000u; i++)
            pk = fabsf(o[i]) > pk ? fabsf(o[i]) : pk;
        CHECK(pk > 0.01f && last[0] > 0u, "PX_RSV 0: the synthesised snare (peak %.3f at 50-90 ms)", pk);
    }
    perc_set_sample(PV_SNARE, 0, 0u, 0u);
}

static void test_adpcm(void)
{
    static int16_t dec[40000];
    static uint8_t a[20000];
    adpcm_state_t st = {0, 0, 0};
    uint32_t n = brk_synth_len(RATE, BPM, 1), i;
    double sig = 0.0, err = 0.0, snr;
    fresh(1);
    if (n > 40000u)
        n = 40000u;
    enc(pcm, a, n);
    adpcm_decode(&st, a, dec, n);
    for (i = 0; i < n; i++) {
        sig += (double)pcm[i] * pcm[i];
        err += (double)(pcm[i] - dec[i]) * (pcm[i] - dec[i]);
    }
    snr = 10.0 * log10(sig / (err + 1.0));
    CHECK(snr > 20.0, "ADPCM round trip of the break: %.1f dB SNR", snr);
    CHECK(n / 2u < 22000u, "a bar at 22.05 kHz is %u bytes of ADPCM", n / 2u);
}

/* the pads' settings, tuned while held (brk_padset) */
static void test_pad_settings(void)
{
    brk_padset_t ps;
    brk_status_t st;
    uint32_t s = brk_step_len_samples(), i;
    float pk[4], peak;
    int32_t seen, sl[16];
    /* roll rate: 1/4 repeats far less often than 1/32 in the same time */
    uint32_t n4, n32;
    fresh(1);
    brk_padset_defaults(&ps);
    ps.roll_div[0] = 0u;                 /* R8 pad set to 1/4 */
    brk_padset(&ps);
    brk_play(1);
    render(buf, s * 2u + 10u, 0, 0, 0);
    brk_pad(PAD_ROLL8, 1);
    render(buf, s * 16u, 0, 0, 0);
    brk_status(&st);
    n4 = st.roll_n;
    brk_pad(PAD_ROLL8, 0);
    ps.roll_div[0] = 3u;                 /* now 1/32 */
    brk_padset(&ps);
    render(buf, s * 2u, 0, 0, 0);
    brk_pad(PAD_ROLL8, 1);
    render(buf, s * 16u, 0, 0, 0);
    brk_status(&st);
    n32 = st.roll_n;
    brk_pad(PAD_ROLL8, 0);
    CHECK(n4 >= 3u && n4 <= 4u && n32 >= 30u, "roll rate follows K1: 1/4 gave %u repeats, 1/32 gave %u", n4, n32);
    /* roll decay: each repeat quieter */
    fresh(1);
    brk_padset_defaults(&ps);
    ps.roll_div[1] = 1u;                 /* R16 pad at 1/8: one repeat every two steps */
    ps.roll_decay = 0.5f;
    brk_padset(&ps);
    brk_play(1);
    render(buf, s * 2u + 10u, 0, 0, 0);
    brk_pad(PAD_ROLL16, 1);
    render(buf, s, 0, 0, 0);             /* to the roll's first hit */
    for (i = 0; i < 4u; i++)
        pk[i] = render(buf, s * 2u, 0, 0, 0);
    brk_pad(PAD_ROLL16, 0);
    CHECK(pk[1] > pk[0] * 0.9f && pk[2] < pk[1] * 0.8f && pk[3] < pk[2] * 0.8f,
          "roll decay: the first repeat at the level, then quieter: %.3f %.3f %.3f %.3f", pk[0], pk[1], pk[2], pk[3]);
    /* roll level: ghost notes, a quarter as loud; and a swell (decay < 0) from there up */
    {
        float full = pk[1], g[4];
        fresh(1);
        brk_padset_defaults(&ps);
        ps.roll_div[1] = 1u;
        ps.roll_level = 25u;
        brk_padset(&ps);
        brk_play(1);
        render(buf, s * 2u + 10u, 0, 0, 0);
        brk_pad(PAD_ROLL16, 1);
        render(buf, s, 0, 0, 0);
        for (i = 0; i < 2u; i++)
            g[i] = render(buf, s * 2u, 0, 0, 0);
        brk_pad(PAD_ROLL16, 0);
        CHECK(g[1] > full * 0.18f && g[1] < full * 0.32f, "roll level 25%%: the repeats a quarter as loud (%.3f of %.3f)",
              g[1], full);
        fresh(1);
        ps.roll_decay = -0.5f;
        brk_padset(&ps);
        brk_play(1);
        render(buf, s * 2u + 10u, 0, 0, 0);
        brk_pad(PAD_ROLL16, 1);
        render(buf, s, 0, 0, 0);
        for (i = 0; i < 4u; i++)
            g[i] = render(buf, s * 2u, 0, 0, 0);
        brk_pad(PAD_ROLL16, 0);
        CHECK(g[1] < full * 0.32f && g[2] > g[1] * 1.3f && g[3] > g[2] * 1.1f,
              "roll swell: from the level up: %.3f %.3f %.3f", g[1], g[2], g[3]);
        brk_padset_defaults(&ps);
        brk_padset(&ps);
    }
    /* tape stop time: a quarter beat stops within two steps */
    fresh(1);
    brk_padset_defaults(&ps);
    ps.stop_beats = 0.25f;
    brk_padset(&ps);
    brk_play(1);
    render(buf, s * 2u, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 1);
    render(buf, s + 64u, 0, 0, 0);
    peak = render(buf, BLOCK, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 0);
    CHECK(peak < 0.01f, "tape stop at 1/4 beat: silent after a step (peak %.3f)", peak);
    /* gate length: 10% open is mostly silence */
    {
        uint32_t loud = 0u, n = s * 8u;
        fresh(1);
        brk_padset_defaults(&ps);
        ps.gate_len = 0.1f;
        brk_padset(&ps);
        brk_play(1);
        render(buf, s, 0, 0, 0);
        brk_pad(PAD_GATE, 1);
        render(buf, n, 0, 0, 0);
        for (i = 0; i < n; i++)
            if (fabsf(buf[i]) > 0.02f)
                loud++;
        brk_pad(PAD_GATE, 0);
        CHECK(loud < n / 4u, "gate 10%%: %u of %u samples sound", loud, n);
    }
    /* random amount 0: the loop as it is */
    {
        int ok = 1;
        fresh(1);
        brk_padset_defaults(&ps);
        ps.rnd_amount = 0.0f;
        brk_padset(&ps);
        brk_play(1);
        brk_pad(PAD_RANDOM, 1);
        render(buf, s * 8u, &seen, sl, 16);
        for (i = 0; i < (uint32_t)seen && i < 8u; i++)
            if (sl[i] != (int32_t)i)
                ok = 0;
        brk_pad(PAD_RANDOM, 0);
        CHECK(ok, "random at 0%% plays the loop");
    }
}

/* the dub delay: echoes ring on after the pad and the break stop */
static void test_dub(void)
{
    static int16_t line[32768];
    brk_fx_t p;
    uint32_t s = brk_step_len_samples(), n = s * 4u, i;
    float tail = 0.0f;
    fresh(1);
    brk_fx_init();
    brk_fx_defaults(&p);
    brk_fx_set(&p);
    brk_fx_delay_buffer(line, 32768u);
    brk_fx_tempo(BPM);
    brk_play(1);
    render(buf, n, 0, 0, 0);
    for (i = 0; i < n; i += BLOCK)
        brk_fx_process(buf + i, 0, 0, 0, 0, bl + i, br + i, n - i < BLOCK ? n - i : BLOCK, 1u << PAD_DUB);
    brk_play(0);
    brk_pad(PAD_DUB, 0);
    for (i = 0; i < BLOCK; i++)
        buf[i] = 0.0f;
    for (i = 0; i < s * 8u; i += BLOCK) {    /* silence in, two bars of it */
        uint32_t j;
        brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
        for (j = 0; j < BLOCK; j++)
            if (fabsf(bl[j]) > tail)
                tail = fabsf(bl[j]);
    }
    CHECK(tail > 0.01f, "dub echoes ring on after the pad (peak %.3f)", tail);
    brk_fx_delay_buffer(0, 0u);
}

/* the built-in styles: each one loads, has 16 slices and plays them in order straight */
static void test_styles(void)
{
    uint32_t k, bad = 0u;
    for (k = 0; k < brk_synth_count(); k++) {
        int32_t seen, sl[20], i, ok = 1;
        brk_init();
        brk_synth_style(pcm, RATE, k, &brk);
        brk_set_break(&brk);
        brk_tempo((float)brk.bpm_x100 / 100.0f);
        brk_play(1);
        render(buf, brk_step_len_samples() * 16u, &seen, sl, 20);
        for (i = 0; i < 16 && i < seen; i++)
            if (sl[i] != i)
                ok = 0;
        if (!ok || seen < 16 || brk.nslices != 16u || brk.len > 81920u)
            bad++;
    }
    CHECK(!bad && brk_synth_count() >= 5u, "%u styles, %u bad", brk_synth_count(), bad);
}

/* the soundboard: every sound sounds, stays inside -1..1 and ends; the siren holds until let go */

/* the pull-up: the break spins back, then starts again from step 0 */

/* the soundboard through the dub delay: echoes after the sound has stopped */

/* the white keys' own loops (brk_keyset): length, ping-pong, one shot, pitch, and the loop back after */
/* LOAF BREAKER (0.9.71, from CHOPPAH 1.5): the split (PRD S-030 to S-032): a 2-bar break of 16 slices, the first 8 bar 1 (break A), the rest bar 2 (break B) */
static void split_fresh(void)
{
    fresh(1);
    brk.bars = 2u;                       /* (the synth's bar of 16 slices played as 2 bars: a slice an 8th) */
    brk.split = 8u;
    brk_set_break(&brk);
}
static void steps_over(uint32_t n, int32_t *lo, int32_t *hi)   /* the steps seen in n frames: their range */
{
    uint32_t i = 0;
    *lo = 99;
    *hi = -1;
    while (i < n) {
        brk_status_t st;
        brk_render(buf, BLOCK);
        brk_status(&st);
        if (st.step >= 0 && st.step < *lo)
            *lo = st.step;
        if (st.step > *hi)
            *hi = st.step;
        i += BLOCK;
    }
}
static float rms_of(const float *x, uint32_t n)
{
    double a = 0.0;
    uint32_t i;
    for (i = 0; i < n; i++)
        a += (double)x[i] * (double)x[i];
    return (float)sqrt(a / (double)n);
}
/* LOAF BREAKER (0.9.75): CHAOS. The Slice Map heard on the test break (kicks, snares, hats); CHAOS swaps a hit for one
 * of its role only, the same seed the same every loop, another seed another; MATH 5 rounds each bar's first five 16ths;
 * off: the loop as ever */
static void chaos_run(uint32_t packed, int32_t *sls, int32_t *seen)
{
    uint32_t sl;
    brk_gen_t g = {0.0f, 0.0f, 0.0f, 0.0f, 0u};   /* (the generator out of it: its fills are chaos of their own) */
    fresh(2);
    brk_gen(&g);
    brk_auto_roles(&brk);
    brk_chaos(packed);
    sl = brk_step_len_samples();
    brk_play(1);
    render(buf, sl * 64u + sl / 2u, seen, sls, 64);
}
static void test_chaos(void)
{
    int32_t a[64], b2[64], c[64], d[64], na, nb, nc, nd, i, k = 0, s = 0, h = 0, ok = 1, diff = 0;
    fresh(2);
    brk_auto_roles(&brk);
    for (i = 0; i < (int32_t)brk.nslices; i++) {
        k += brk.roles[i] >> BRK_ROLE_KICK & 1;
        s += brk.roles[i] >> BRK_ROLE_SNARE & 1;
        h += brk.roles[i] >> BRK_ROLE_HAT & 1;
    }
    CHECK(k > 0 && s > 0 && h > 0, "the Slice Map hears kicks %d, snares %d, hats %d", k, s, h);
    chaos_run(0u, a, &na);
    chaos_run(100u | 1u << 24, b2, &nb);
    for (i = 0; i < na && i < nb; i++) {
        if (a[i] < 0 || b2[i] < 0 || a[i] == b2[i])
            continue;
        diff++;
        if (!(brk.roles[a[i]] & brk.roles[b2[i]]))
            ok = 0;
    }
    CHECK(diff > 4 && ok, "CHAOS 100: %d hits swapped, each for one of its role", diff);
    chaos_run(100u | 1u << 24, c, &nc);
    for (ok = nc == nb, i = 0; ok && i < nc; i++)
        ok = c[i] == b2[i];
    CHECK(ok, "the same seed, the same chaos");
    {
        int32_t per = 1;                 /* (the loop's length in the steps seen: where the plain loop repeats) */
        for (; per < na / 2; per++) {
            for (ok = 1, i = 0; i + per < na; i++)
                if (a[i] != a[i + per])
                    ok = 0;
            if (ok)
                break;
        }
        for (ok = per < na / 2, i = 0; ok && i + per < nb; i++)
            ok = b2[i] == b2[i + per];
        CHECK(ok, "and the same every loop, %d steps seen (it grooves)", per);
    }
    chaos_run(100u | 7u << 24, d, &nd);
    for (diff = 0, i = 0; i < nd && i < nb; i++)
        diff += d[i] != b2[i];
    CHECK(diff > 2, "another seed, another chaos (%d steps differ)", diff);
    chaos_run(5u << 8, d, &nd);
    for (ok = 1, i = 1; i + 5 < 16 && i + 5 < nd; i++)
        if (d[i] >= 0 && d[i + 5] >= 0 && d[i] != d[i + 5])
            ok = 0;
    CHECK(ok, "MATH 5: the bar's first five 16ths round again (%d %d %d / %d %d %d)", d[1], d[2], d[3], d[6], d[7], d[8]);
    chaos_run(0u, c, &nc);
    for (ok = nc == na, i = 0; ok && i < nc; i++)
        ok = c[i] == a[i];
    CHECK(ok, "CHAOS off: the loop as ever");
}

static void test_split(void)
{
    uint32_t sl = brk_step_len_samples(), i;
    int32_t lo, hi, seen, sls[64], ok;
    brk_status_t st;
    float both, one;
    /* one half alone: from the next bar line the loop rounds bar 1 only, its slices those of the first half */
    split_fresh();
    brk_play(1);
    brk_half(0);
    render(buf, sl * 16u, 0, 0, 0);      /* (to the first bar line) */
    steps_over(sl * 48u, &lo, &hi);
    CHECK(lo == 0 && hi == 15, "HALF 1: three bars on steps %d..%d (0..15)", lo, hi);
    render(buf, sl * 32u, &seen, sls, 64);
    for (ok = 1, i = 1; i < (uint32_t)seen; i++)
        if (sls[i] < 0 || sls[i] >= 8)
            ok = 0;
    CHECK(ok && seen > 20, "and only the first half's slices sound");
    /* the second half asked for mid-bar: it waits for the bar line */
    render(buf, sl * 4u + sl / 2u, 0, 0, 0);
    brk_status(&st);
    brk_half(1);
    {
        int32_t at = st.step;
        steps_over(sl * (uint32_t)(15 - at), &lo, &hi);
        CHECK(hi <= 15, "HALF 2 asked mid-bar (step %d): bar 1 plays out (up to %d)", at, hi);
    }
    render(buf, sl, 0, 0, 0);            /* (over the bar line) */
    steps_over(sl * 33u, &lo, &hi);
    CHECK(lo == 16 && hi == 31, "then bar 2 alone: steps %d..%d (16..31)", lo, hi);
    brk_status(&st);
    CHECK(st.half == 1, "the status says half 2 (%d)", st.half);
    /* the generator's slices fold into the half */
    {
        brk_gen_t g = {0.9f, 0.0f, 0.0f, 0.0f, 0u};
        brk_gen(&g);
        render(buf, sl * 64u, &seen, sls, 64);
        for (ok = 1, i = 1; i < (uint32_t)seen; i++)
            if (sls[i] >= 0 && sls[i] < 8)
                ok = 0;
        CHECK(ok && seen > 20, "COMPLEXITY's jumps stay in half 2");
        g.complexity = 0.0f;
        brk_gen(&g);
    }
    /* both again */
    brk_half(-1);
    steps_over(sl * 80u, &lo, &hi);
    CHECK(lo == 0 && hi == 31, "both halves again: steps %d..%d", lo, hi);
    /* no split: the half asked for does nothing */
    fresh(1);
    brk.bars = 2u;
    brk_set_break(&brk);
    brk_play(1);
    brk_half(0);
    steps_over(sl * 80u, &lo, &hi);
    CHECK(lo == 0 && hi == 31, "no split: HALF ignored (steps %d..%d)", lo, hi);

    /* layers, stopped: a key in each half sounds together */
    split_fresh();
    brk_key(2, 1);
    render(buf, sl * 2u, 0, 0, 0);
    one = rms_of(buf, sl * 2u);
    brk_key(11, 1);
    render(buf, sl * 2u, 0, 0, 0);
    both = rms_of(buf, sl * 2u);
    brk_status(&st);
    CHECK(st.key == 2 && st.layer == 11, "a key in each half: key %d loops, %d layers", st.key, st.layer);
    CHECK(both > one * 1.15f, "and both sound (rms %.4f, one alone %.4f)", both, one);
    CHECK(st.layer_lo == (float)brk.slice[11] && st.layer_pos >= 0.0f, "the layer loops its own window (from %.0f)",
          st.layer_lo);
    brk_key(5, 1);                       /* the first half again: it takes over that half, the layer stays */
    render(buf, sl, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == 5 && st.layer == 11, "a second key in half 1 takes it over (key %d, layer %d)", st.key, st.layer);
    brk_key(13, 1);
    render(buf, sl, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == 5 && st.layer == 13, "and one in half 2 the layer (key %d, layer %d)", st.key, st.layer);
    brk_key(13, 0);
    render(buf, sl, 0, 0, 0);
    brk_status(&st);
    CHECK(st.layer == 11, "the layer let go: 11, still held, takes it again (%d)", st.layer);
    brk_key(11, 0);
    brk_key(5, 0);
    brk_key(2, 0);
    render(buf, sl * 4u, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == -1 && st.layer == -1 && render(buf, BLOCK, 0, 0, 0) == 0.0f, "all let go: quiet");
    /* playing: the layer lands on the snap too, over the key looping */
    split_fresh();
    brk_play(1);
    render(buf, sl / 2u, 0, 0, 0);
    brk_key(1, 1);
    render(buf, sl * 2u, 0, 0, 0);
    brk_key(12, 1);
    brk_status(&st);
    CHECK(st.layer == -1, "playing: the layer waits for its snap");
    render(buf, sl * 3u, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == 1 && st.layer == 12, "then layers (key %d, layer %d)", st.key, st.layer);
    brk_key(1, 0);
    render(buf, sl * 3u, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == -1 && st.layer == 12, "the key let go, the layer loops on over the loop (%d)", st.layer);
    brk_key(12, 0);
    /* no split: the last key wins, as ever */
    fresh(1);
    brk_key(2, 1);
    brk_key(11, 1);
    render(buf, sl, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == 11 && st.layer == -1, "no split: the last key wins (key %d, layer %d)", st.key, st.layer);
    brk_key(2, 0);
    brk_key(11, 0);
}

static void test_key_loops(void)
{
    brk_keyset_t k;
    brk_status_t st;
    uint32_t sl = brk_step_len_samples(), i;
    float peak, p0;
    int rev_seen[2] = {0, 0};
    /* 4 slices (= 4 16ths of a 1-bar break), stopped: it restarts every 4 steps on the tempo */
    fresh(1);
    brk_keyset_defaults(&k);
    k.len = 5u;
    brk_keyset(2u, &k);
    brk_key(2, 1);
    render(buf, sl * 9u / 2u, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == 2 && st.key_n == 1u, "a held key loops (key %d, %u repeats after 4.5 steps)", st.key, st.key_n);
    CHECK(st.key_phase > 0.05f && st.key_phase < 0.25f, "and is an eighth into its second repeat (%.2f)", st.key_phase);
    CHECK(st.slice == 2 && st.pos >= (float)brk.slice[2] && st.pos < (float)brk.slice[6], "inside its 4-slice window");
    brk_key(2, 0);
    render(buf, sl * 5u, 0, 0, 0);
    brk_status(&st);
    CHECK(st.key == -1 && render(buf, BLOCK, 0, 0, 0) == 0.0f, "let go (stopped): it ends with its window");
    /* the jungle chops on a 2-bar break of 16 slices (an 8th each, as the bank's): 1.5 slices restart every 3 16ths
     * (3/16), 3 every 6 (3/8) (CHOPPAH 1.3.1's test) */
    {
        static const uint8_t LEN[2] = {8u, 9u};
        static const uint32_t N[2] = {2u, 1u};
        static const float PH[2] = {1.0f / 3.0f, 1.0f / 6.0f};
        for (i = 0; i < 2u; i++) {
            fresh(1);
            brk.bars = 2u;               /* (the synth's bar of 16 slices played as 2 bars: a slice an 8th) */
            brk_set_break(&brk);
            brk_keyset_defaults(&k);
            k.len = LEN[i];
            brk_keyset(3u, &k);
            brk_key(3, 1);
            render(buf, sl * 7u, 0, 0, 0);
            brk_status(&st);
            CHECK(st.key == 3 && st.key_n == N[i] && fabsf(st.key_phase - PH[i]) < 0.06f,
                  "length %s: %u repeats in 7 16ths, %.2f into the next", i ? "3/8" : "3/16", st.key_n, st.key_phase);
            brk_key(3, 0);
        }
    }
    /* ping-pong: every other repeat backwards */
    fresh(1);
    brk_keyset_defaults(&k);
    k.mode = KEY_PINGPONG;
    brk_keyset(5u, &k);
    brk_key(5, 1);
    for (i = 0; i < 6u; i++) {
        render(buf, sl / 2u, 0, 0, 0);
        brk_status(&st);
        rev_seen[st.key_rev ? 1 : 0] |= 1 << (st.key_n & 1u);
        render(buf, sl / 2u, 0, 0, 0);
    }
    CHECK(rev_seen[0] == 1 && rev_seen[1] == 2, "ping-pong: even repeats forwards, odd ones backwards (%d %d)",
          rev_seen[0], rev_seen[1]);
    brk_key(5, 0);
    /* one shot: once through, then quiet while still held */
    fresh(1);
    brk_keyset_defaults(&k);
    k.mode = KEY_ONESHOT;
    brk_keyset(4u, &k);
    brk_key(4, 1);
    peak = render(buf, sl, 0, 0, 0);
    render(buf, sl / 4u, 0, 0, 0);
    CHECK(peak > 0.05f && render(buf, sl * 2u, 0, 0, 0) < 0.001f, "one shot: plays (%.2f), then quiet while held", peak);
    k.mode = KEY_LOOP;                   /* turned back to a loop while held: it loops again */
    brk_keyset(4u, &k);
    render(buf, sl, 0, 0, 0);
    CHECK(render(buf, sl, 0, 0, 0) > 0.05f, "a one shot turned back to a loop while held loops again");
    brk_key(4, 0);
    /* pitch: +12 runs through the break twice as fast */
    fresh(1);
    brk_key(0, 1);
    render(buf, BLOCK, 0, 0, 0);
    brk_status(&st);
    p0 = st.pos;
    render(buf, BLOCK * 4u, 0, 0, 0);
    brk_status(&st);
    p0 = st.pos - p0;
    brk_key(0, 0);
    fresh(1);
    brk_keyset_defaults(&k);
    k.pitch = 12;
    brk_keyset(0u, &k);
    brk_key(0, 1);
    render(buf, BLOCK, 0, 0, 0);
    brk_status(&st);
    peak = st.pos;
    render(buf, BLOCK * 4u, 0, 0, 0);
    brk_status(&st);
    CHECK(fabsf((st.pos - peak) / p0 - 2.0f) < 0.05f, "a key's +12 plays at twice the speed (%.2f)", (st.pos - peak) / p0);
    brk_key(0, 0);
    /* playing: a whole-bar loop from slice 8 owns the voice; let go, the loop takes over at the next slice */
    {
        int32_t seen, sls[40], ok = 1;
        fresh(1);
        brk_keyset_defaults(&k);
        k.len = 7u;                      /* 16 slices: the whole bar */
        brk_keyset(8u, &k);
        brk_play(1);
        render(buf, sl / 2u, 0, 0, 0);
        brk_key(8, 1);
        render(buf, sl * 20u, &seen, sls, 40);
        brk_status(&st);
        for (i = 1; i < (uint32_t)seen; i++)
            if (sls[i] != 8)
                ok = 0;
        CHECK(ok && st.key == 8 && st.key_n == 1u, "playing, a 1-bar key loop holds for 20 steps (repeats %u)", st.key_n);
        brk_key(8, 0);
        render(buf, sl * 2u, &seen, sls, 40);
        brk_status(&st);
        CHECK(st.key == -1 && st.slice != 8, "let go, the loop plays on (slice %d)", st.slice);
    }
}

/* MIDI clock: 24 straight ticks a beat out (swing or not); in, the loop eases onto an outside clock's 16ths */
static void test_clock(void)
{
    uint16_t off[64];
    uint32_t sl = brk_step_len_samples(), t, n, total, k, last = 0, bad = 0;
    int pass;
    for (pass = 0; pass < 2; pass++) {
        fresh(1);
        brk_swing(pass ? 0.66f : 0.5f);
        brk_play(1);
        total = 0;
        for (t = 0; t < sl * 16u; t += 64u) {   /* one bar */
            brk_render(buf, 64);
            n = brk_clock_ticks(off, 64);
            for (k = 0; k < n; k++) {
                uint32_t at = t + off[k];
                if (at >= sl * 16u)
                    break;               /* (the next bar's first) */
                if (total && (at - last > sl / 6u + 2u || at - last < sl / 6u - 2u))
                    bad++;
                last = at;
                total++;
            }
        }
        CHECK(total == 96u && !bad, "clock out: %u ticks a bar (96), %u uneven%s", total, bad, pass ? ", with swing" : "");
    }
    /* in: the clock's 16ths begin 0.4 of a step after the loop's; synced every beat, the loop moves onto them */
    {
        uint32_t ext0 = sl * 2u / 5u, beat = sl * 4u, nb = 0;
        int32_t last_step = -9, d = 99999;
        brk_status_t st;
        fresh(1);
        brk_play(1);
        for (t = 0; t < beat * 12u; t += 16u) {
            if (t >= ext0 && (t - ext0) / beat >= nb) {   /* a beat of the outside clock began in the last block */
                uint32_t at = ext0 + nb * beat;
                brk_sync(nb * 4u, (float)(t - at));
                nb++;
            }
            brk_render(buf, 16);
            brk_status(&st);
            if (st.step != last_step) {
                last_step = st.step;
                if (t > beat * 10u) {    /* the loop's step boundary against the clock's nearest */
                    int32_t rel = (int32_t)((t + 16u - ext0) % sl);
                    if (rel > (int32_t)sl / 2)
                        rel -= (int32_t)sl;
                    if (rel < 0 ? -rel < (d < 0 ? -d : d) : rel < (d < 0 ? -d : d))
                        d = rel;
                }
            }
        }
        CHECK(d > -48 && d < 48, "clock in: the loop's steps land on the clock's (%d samples off, was %u)", d, ext0);
    }
    /* far off: it jumps to where the clock is */
    {
        brk_status_t st;
        fresh(1);
        brk_play(1);
        render(buf, sl * 3u + sl / 2u, 0, 0, 0);
        brk_sync(40u, 0.0f);
        render(buf, sl / 2u, 0, 0, 0);
        brk_status(&st);
        CHECK(st.step == 41 % 16, "clock in, far off: the loop jumps to the clock's 16th (step %d, want %d)", st.step, 41 % 16);
    }
}

/* pitch, KEEP LENGTH: a 1 kHz tone pitched +12 plays at 2 kHz, but the voice runs at its own speed */
static void test_keep_length(void)
{
    static brk_break_t tone;
    uint32_t i, n = 22050u, zc[2], mode;
    float adv[2];
    for (i = 0; i < n; i++)
        pcm[i] = (int16_t)(12000.0f * sinf(6.2831853f * (float)i * (1000.0f / 22050.0f)));
    for (mode = 0; mode < 2u; mode++) {
        brk_status_t st;
        float p0;
        uint32_t k;
        brk_init();
        tone.pcm = pcm;
        tone.len = n;
        tone.rate = 22050u;
        tone.bpm_x100 = 12000u;
        tone.bars = 1u;
        tone.nslices = 1u;
        tone.slice[0] = 0u;
        tone.slice[1] = n;
        brk_set_break(&tone);
        brk_tempo(120.0f);
        brk_pitch(12.0f);
        brk_pitch_mode((int)mode, 30.0f);
        brk_key(0, 1);
        render(buf, 4096u, 0, 0, 0);
        brk_status(&st);
        p0 = st.pos;
        render(buf, 8192u, 0, 0, 0);
        brk_status(&st);
        adv[mode] = (st.pos - p0) / (8192.0f * 0.5f);   /* 1: the voice at its own speed (22.05 kHz into 44.1) */
        for (k = 1, zc[mode] = 0; k < 8192u; k++)
            if ((buf[k - 1] < 0.0f) != (buf[k] < 0.0f))
                zc[mode]++;
        brk_key(0, 0);
    }
    brk_pitch(0.0f);
    brk_pitch_mode(0, 30.0f);
    CHECK(adv[0] > 1.9f && adv[0] < 2.1f && zc[0] > 700u && zc[0] < 790u,
          "pitch SPEED: +12 runs twice as fast (%.2f) at 2 kHz (%u crossings)", adv[0], zc[0]);
    CHECK(adv[1] > 0.95f && adv[1] < 1.05f && zc[1] > 690u && zc[1] < 800u,
          "pitch KEEP LENGTH: +12 runs at its own speed (%.2f) but sounds at 2 kHz (%u crossings)", adv[1], zc[1]);
}

/* GLOUPI BREAKS: HALF SPEED's master tempo: a 1 kHz tone held under HALF SPEED runs at half its speed but still
 * sounds at 1 kHz, in SPEED and in KEEP LENGTH */
static void test_half_master(void)
{
    static brk_break_t tone;
    uint32_t i, n = 22050u, zc[3], mode;
    float adv[3];
    for (i = 0; i < n; i++)
        pcm[i] = (int16_t)(12000.0f * sinf(6.2831853f * (float)i * (1000.0f / 22050.0f)));
    for (mode = 0; mode < 3u; mode++) {  /* SPEED, KEEP LENGTH; and SPEED with the master tempo off */
        brk_status_t st;
        float p0;
        uint32_t k;
        brk_init();
        brk_half_master(mode < 2u);
        tone.pcm = pcm;
        tone.len = n;
        tone.rate = 22050u;
        tone.bpm_x100 = 12000u;
        tone.bars = 1u;
        tone.nslices = 1u;
        tone.slice[0] = 0u;
        tone.slice[1] = n;
        brk_set_break(&tone);
        brk_tempo(120.0f);
        brk_pitch(0.0f);
        brk_pitch_mode((int)(mode == 1u), 30.0f);
        brk_key(0, 1);
        brk_pad(PAD_HALF, 1);
        render(buf, 4096u, 0, 0, 0);
        brk_status(&st);
        p0 = st.pos;
        render(buf, 8192u, 0, 0, 0);
        brk_status(&st);
        adv[mode] = (st.pos - p0) / (8192.0f * 0.5f);   /* 1: the voice at its own speed */
        for (k = 1, zc[mode] = 0; k < 8192u; k++)
            if ((buf[k - 1] < 0.0f) != (buf[k] < 0.0f))
                zc[mode]++;
        brk_pad(PAD_HALF, 0);
        brk_key(0, 0);
    }
    brk_pitch_mode(0, 30.0f);
    CHECK(adv[0] > 0.45f && adv[0] < 0.55f && zc[0] > 330u && zc[0] < 410u,
          "HALF SPEED, SPEED: half as fast (%.2f), still 1 kHz (%u crossings)", adv[0], zc[0]);
    CHECK(adv[1] > 0.45f && adv[1] < 0.55f && zc[1] > 330u && zc[1] < 410u,
          "HALF SPEED, KEEP LENGTH: half as fast (%.2f), still 1 kHz (%u crossings)", adv[1], zc[1]);
    CHECK(adv[2] > 0.45f && adv[2] < 0.55f && zc[2] > 150u && zc[2] < 220u,
          "HALF SPEED, master tempo off: half as fast (%.2f), an octave down (%u crossings)", adv[2], zc[2]);
    brk_half_master(1);
}

/* the gate: at 25 % each slice of the loop sounds for a quarter of its length, then it is quiet until the next */
static void test_gate(void)
{
    uint32_t sl, i;
    float early[2], late[2];
    int g;
    for (g = 0; g < 2; g++) {
        fresh(1);                        /* 1 bar, 16 slices: a slice a step */
        sl = brk_step_len_samples();     /* (at fresh's tempo) */
        brk_gate(g ? 0.25f : 1.0f);
        brk_play(1);
        render(buf, sl * 4u, 0, 0, 0);   /* into step 4 */
        early[g] = late[g] = 0.0f;
        for (i = 0; i < 4u; i++) {       /* in each of the next steps: its first fifth, and its second half */
            render(buf, sl, 0, 0, 0);
            {
                uint32_t k;
                for (k = 0; k < sl / 5u; k++)
                    if (fabsf(buf[k]) > early[g])
                        early[g] = fabsf(buf[k]);
                for (k = sl / 2u; k < sl; k++)
                    if (fabsf(buf[k]) > late[g])
                        late[g] = fabsf(buf[k]);
            }
        }
    }
    brk_gate(1.0f);
    CHECK(early[1] > early[0] * 0.8f && late[1] < 0.002f && late[0] > 0.01f,
          "gate 25%%: each slice's start sounds (%.3f), its second half is quiet (%.4f; ungated %.3f)",
          early[1], late[1], late[0]);
}

/* a break change (ALGORITHM): muted, the break runs on silent; the next one swapped in waits for the next step,
 * then sounds; stopped, it is in at once */
static void test_swap(void)
{
    static int16_t pcm2[200000];
    static brk_break_t b2;
    uint32_t sl;
    float pk;
    fresh(1);
    brk_synth_break(pcm2, RATE, BPM, 1u, &b2);
    sl = brk_step_len_samples();
    brk_play(1);
    render(buf, sl * 3u + sl / 4u, 0, 0, 0);     /* a quarter into step 3 */
    brk_mute(1);
    render(buf, 512u, 0, 0, 0);                   /* (the fade: two renders) */
    pk = render(buf, sl / 4u, 0, 0, 0);
    CHECK(pk < 1e-4f, "muted: silent (%.5f)", pk);
    brk_swap_break(&b2);
    render(buf, sl / 8u, 0, 0, 0);
    CHECK(brk_get_break() == &brk, "the new break waits for the next step");
    pk = render(buf, sl, 0, 0, 0);                /* past step 4's start */
    CHECK(brk_get_break() == &b2 && pk > 0.01f, "in on the next step, and sounding (%.3f)", pk);
    brk_play(0);
    render(buf, 1024u, 0, 0, 0);
    brk_swap_break(&brk);
    render(buf, 256u, 0, 0, 0);
    CHECK(brk_get_break() == &brk, "stopped: in at once");
}

/* the snap: where a key pressed while playing begins. A 1-bar break of 16 slices: a slice is a 16th */
static void test_snap(void)
{
    uint32_t sl, t;
    int32_t at;
    brk_status_t st;
    /* 2 slices: pressed in step 0, it waits for step 2 (not 1) */
    fresh(1);
    sl = brk_step_len_samples();
    brk_snap(2.0f);
    brk_play(1);
    render(buf, sl / 2u, 0, 0, 0);
    brk_key(9, 1);
    brk_key(9, 0);
    render(buf, sl, 0, 0, 0);            /* well into step 1 */
    brk_status(&st);
    CHECK(st.step == 1 && st.slice == 1, "snap 2 slices: not yet at step 1 (slice %d)", st.slice);
    render(buf, sl, 0, 0, 0);            /* into step 2 */
    brk_status(&st);
    CHECK(st.step == 2 && st.slice == 9, "snap 2 slices: the key begins at step 2 (slice %d)", st.slice);
    /* OFF: at once, mid-step; and a quarter slice: a quarter step on */
    {
        float q[2] = {0.0f, 0.25f};
        int k;
        for (k = 0; k < 2; k++) {
            fresh(1);
            sl = brk_step_len_samples();
            brk_snap(q[k]);
            brk_play(1);
            render(buf, sl + sl / 10u, 0, 0, 0);   /* a tenth into step 1 */
            brk_key(12, 1);
            for (t = 0, at = -1; t < sl; t += 16u) {
                brk_render(buf, 16);
                brk_status(&st);
                if (st.slice == 12) {
                    at = (int32_t)t;
                    break;
                }
            }
            brk_key(12, 0);
            if (k == 0)
                CHECK(at >= 0 && at < 64, "snap OFF: the key at once (%d samples)", at);
            else
                CHECK(at > (int32_t)(sl * 3u / 20u) - 64 && at < (int32_t)(sl * 3u / 20u) + 64,
                      "snap 1/4 slice: the key a quarter step in (%d samples, want %u)", at, sl * 3u / 20u);
        }
    }
    brk_snap(1.0f);
}

/* the reverb: an impulse rings on, decays, comes out different left and right; a bigger size rings longer; a second
 * REVERB slot passes the sound on */
static float rv_energy(float size, uint32_t from, uint32_t to, int two, float *corr)
{
    brk_fx_t p;
    uint32_t i, n = BRK_SR * 4u;
    float e = 0.0f, lr = 0.0f, ll = 0.0f, rr = 0.0f;
    for (i = 0; i < n; i++)
        buf[i] = i == 100u ? 1.0f : 0.0f;
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], FXT_REVERB);
    p.slot[0].on = 1u;
    p.slot[0].mix = 1.0f;
    p.slot[0].p[0] = size;
    p.slot[0].p[2] = 0.0f;
    if (two) {
        p.slot[2] = p.slot[0];
    }
    fx_run(&p, n);
    for (i = from; i < to; i++) {
        e += bl[i] * bl[i] + br[i] * br[i];
        lr += bl[i] * br[i];
        ll += bl[i] * bl[i];
        rr += br[i] * br[i];
    }
    if (corr)
        *corr = lr / sqrtf(ll * rr + 1e-12f);
    return e;
}

static void test_reverb(void)
{
    float early, late, big_late, corr, one, two;
    early = rv_energy(0.6f, BRK_SR / 10u, BRK_SR / 2u, 0, &corr);
    late = rv_energy(0.6f, BRK_SR * 2u, BRK_SR * 3u, 0, 0);
    big_late = rv_energy(1.0f, BRK_SR * 2u, BRK_SR * 3u, 0, 0);
    CHECK(early > 1e-3f && late < early * 0.05f, "reverb: an impulse rings (%.4f) and dies away (%.6f 2 s on)", early, late);
    CHECK(corr < 0.5f && corr > -0.5f, "reverb: left and right differ (correlation %.2f)", corr);
    CHECK(big_late > late * 4.0f, "reverb: a bigger size rings longer (%.6f against %.6f 2 s on)", big_late, late);
    one = rv_energy(0.6f, 0u, BRK_SR, 0, 0);
    two = rv_energy(0.6f, 0u, BRK_SR, 1, 0);
    CHECK(fabsf(two - one) < one * 0.01f, "reverb: a second REVERB slot passes the sound on (%.4f, %.4f)", one, two);
    {   /* a listen: the break through the plate */
        brk_fx_t p;
        uint32_t n = BRK_SR * 4u;
        fresh(1);
        brk_play(1);
        render(buf, n, 0, 0, 0);
        brk_fx_defaults(&p);
        brk_fx_slot_defaults(&p.slot[0], FXT_REVERB);
        p.slot[0].on = 1u;
        p.slot[0].mix = 0.5f;
        fx_run(&p, n);
        wav("reverb.wav", bl, br, n);
    }
}

/* STOP: the break fades out at once (a key held loops on) */
static void test_stop(void)
{
    fresh(1);
    brk_play(1);
    render(buf, brk_step_len_samples() * 4u + 100u, 0, 0, 0);
    brk_play(0);
    render(buf, 441u, 0, 0, 0);          /* 10 ms */
    CHECK(render(buf, BLOCK * 4u, 0, 0, 0) == 0.0f, "stop: the break is quiet 10 ms on");
    fresh(1);
    brk_play(1);
    render(buf, brk_step_len_samples() * 2u, 0, 0, 0);
    brk_play(0);
    render(buf, BLOCK, 0, 0, 0);
    brk_key(5, 1);                       /* stopped, a key still plays and loops */
    render(buf, brk_step_len_samples() * 3u, 0, 0, 0);
    CHECK(render(buf, brk_step_len_samples(), 0, 0, 0) > 0.05f, "stop: a key pressed after still loops");
    brk_key(5, 0);
}

/* the DELAY slot: a click comes back on the left at the time, then on the right, ping-pong; and the slices' own
 * effects: a level of 0 silences that slice alone, a LO-PASS darkens it, an ECHO throws it alone into the delay */
static int16_t dly_line[32768];
static void test_delay_slices(void)
{
    brk_fx_t p;
    uint32_t i, n = BRK_SR * 2u, T = (uint32_t)(3.0f * 22050.0f * 60.0f / (170.0f * 4.0f)) * 2u, pkl = 0, pkr = 0;
    float ml = 0.0f, mr = 0.0f;
    brk_fx_init();
    brk_fx_delay_buffer(dly_line, 32768u);
    brk_fx_tempo(170.0f);
    for (i = 0; i < n; i++)
        buf[i] = i == 1000u ? 1.0f : 0.0f;
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], FXT_DELAY);   /* 3/16 */
    p.slot[0].on = 1u;
    p.slot[0].mix = 1.0f;
    brk_fx_set(&p);
    for (i = 0; i < n; i += BLOCK)
        brk_fx_process(buf + i, 0, 0, 0, 0, bl + i, br + i, BLOCK, 0u);
    for (i = 1100u; i < 1000u + T + T / 2u; i++) {   /* the first echo: left only */
        if (fabsf(bl[i]) > ml) { ml = fabsf(bl[i]); pkl = i; }
        if (fabsf(br[i]) > mr) mr = fabsf(br[i]);
    }
    CHECK(ml > 0.02f && mr < ml * 0.1f && pkl > 1000u + T - 8u && pkl < 1000u + T + 8u,
          "delay: the first echo on the left at 3/16 (%u samples on, want %u; right %.3f)", pkl - 1000u, T, mr);
    ml = mr = 0.0f;
    for (i = 1000u + T + T / 2u; i < 1000u + 2u * T + T / 2u; i++) {   /* the second: right */
        if (fabsf(br[i]) > mr) { mr = fabsf(br[i]); pkr = i; }
        if (fabsf(bl[i]) > ml) ml = fabsf(bl[i]);
    }
    CHECK(mr > 0.02f && ml < mr * 0.1f && pkr > 1000u + 2u * T - 12u && pkr < 1000u + 2u * T + 12u,
          "delay: the second on the right, ping-pong (%u on)", pkr - 1000u);
    brk_fx_delay_buffer(0, 0u);
    /* the slices' own effects, through the engine */
    {
        brk_slicefx_t f;
        uint32_t sl, k, zc[2];
        float lv[2], e_on, e_off;
        static float ebuf[BRK_SR * 2u];
        for (k = 0; k < 2u; k++) {       /* level: slice 4 at 0 */
            fresh(1);
            sl = brk_step_len_samples();
            brk_slicefx_defaults(&f);
            f.level = k ? 0u : 100u;
            brk_slicefx(4u, &f);
            brk_play(1);
            render(buf, sl * 4u + sl / 10u, 0, 0, 0);
            lv[k] = render(buf, sl * 8u / 10u, 0, 0, 0);    /* inside step 4 */
        }
        CHECK(lv[0] > 0.05f && lv[1] < 0.001f, "slice level 0: that slice silent (%.3f, was %.3f)", lv[1], lv[0]);
        for (k = 0; k < 2u; k++) {       /* LO-PASS on slice 2 (a snare's hiss): fewer zero crossings */
            uint32_t j;
            fresh(1);
            sl = brk_step_len_samples();
            brk_slicefx_defaults(&f);
            f.type = k ? SLX_LOPASS : SLX_OFF;
            f.amount = 85u;
            brk_slicefx(2u, &f);
            brk_play(1);
            render(buf, sl * 2u + 64u, 0, 0, 0);
            render(buf, sl / 2u, 0, 0, 0);
            for (j = 1, zc[k] = 0; j < sl / 2u; j++)
                if ((buf[j - 1] < 0.0f) != (buf[j] < 0.0f))
                    zc[k]++;
        }
        CHECK(zc[1] < zc[0] / 2u, "slice LO-PASS: darker (%u crossings, was %u)", zc[1], zc[0]);
        /* ECHO on slice 0 only: its throw is there while slice 0 plays, and nothing while slice 1 does */
        fresh(1);
        sl = brk_step_len_samples();
        brk_slicefx_defaults(&f);
        f.type = SLX_ECHO;
        f.amount = 100u;
        brk_slicefx(0u, &f);
        brk_sends(ebuf, 0);
        brk_play(1);
        render(buf, sl / 2u, 0, 0, 0);
        for (i = 0, e_on = 0.0f; i < sl / 2u; i++)
            e_on += fabsf(ebuf[i]);
        render(buf, sl / 2u + 64u, 0, 0, 0);
        render(buf, sl / 2u, 0, 0, 0);
        for (i = 0, e_off = 0.0f; i < sl / 2u; i++)
            e_off += fabsf(ebuf[i]);
        brk_sends(0, 0);
        CHECK(e_on > 1.0f && e_off < 0.01f, "slice ECHO: slice 0 thrown (%.2f), slice 1 not (%.4f)", e_on, e_off);
    }
}

/* EQ: flat at 0.5; LOW +12 dB lifts 60 Hz four times, HIGH -12 dB drops 12 kHz to a quarter. TAPE: drive puts
 * harmonics on a pure tone; wow makes its pitch wander */
static float tone_peak_a(float hz, float amp, int type, float p0, float p1, float p2, float mix, uint32_t from)
{
    brk_fx_t p;
    uint32_t i, n = BRK_SR;
    float pk = 0.0f;
    for (i = 0; i < n; i++)
        buf[i] = amp * sinf(6.2831853f * hz * (float)i / (float)BRK_SR);
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], type);
    p.slot[0].on = 1u;
    p.slot[0].p[0] = p0;
    p.slot[0].p[1] = p1;
    p.slot[0].p[2] = p2;
    p.slot[0].mix = mix;
    fx_run(&p, n);
    for (i = from; i < n; i++)
        if (fabsf(bl[i]) > pk)
            pk = fabsf(bl[i]);
    return pk;
}

static float tone_peak(float hz, int type, float p0, float p1, float p2, float mix, uint32_t from)
{
    return tone_peak_a(hz, 0.25f, type, p0, p1, p2, mix, from);
}

static void test_eq_tape(void)
{
    float flat = tone_peak(1000.0f, FXT_EQ, 0.5f, 0.5f, 0.5f, 0.5f, 4000u);
    float low = tone_peak(60.0f, FXT_EQ, 1.0f, 0.5f, 0.5f, 0.5f, 8000u);
    float high = tone_peak(12000.0f, FXT_EQ, 0.5f, 0.5f, 0.0f, 0.5f, 4000u);
    float mid = tone_peak(1000.0f, FXT_EQ, 0.5f, 1.0f, 0.5f, 0.5f, 4000u);
    CHECK(fabsf(flat - 0.25f) < 0.01f, "EQ flat: a 1 kHz tone through unchanged (%.3f of 0.25)", flat);
    CHECK(low > 0.85f && low < 1.15f, "EQ LOW +12 dB: 60 Hz four times (%.3f from 0.25)", low);
    CHECK(high > 0.05f && high < 0.08f, "EQ HIGH -12 dB: 12 kHz to a quarter (%.3f from 0.25)", high);
    CHECK(mid > 0.85f && mid < 1.15f, "EQ MID +12 dB at 1 kHz: four times (%.3f)", mid);
    {   /* TAPE: the drive's harmonics (what is left after a notch at the tone), and the wow's wandering */
        uint32_t i, last = 0, nz = 0;
        float lo = 1e9f, hi = 0.0f, res = 0.0f, x1 = 0.0f, x2 = 0.0f;
        float w0 = 6.2831853f * 1000.0f / (float)BRK_SR, cw = 2.0f * cosf(w0);
        tone_peak(1000.0f, FXT_TAPE, 1.0f, 0.0f, 0.0f, 1.0f, 0u);
        for (i = 4000u; i < BRK_SR; i++) {   /* a notch at 1 kHz: x[n] - 2cos(w) x[n-1] + x[n-2] */
            float y = bl[i] - cw * x1 + x2;
            x2 = x1;
            x1 = bl[i];
            if (i > 4010u && fabsf(y) > res)
                res = fabsf(y);
        }
        CHECK(res > 0.02f, "TAPE drive: harmonics on a pure tone (%.3f left past a notch at it)", res);
        tone_peak(1000.0f, FXT_TAPE, 0.0f, 0.0f, 1.0f, 1.0f, 0u);
        for (i = 4001u; i < BRK_SR; i++)  /* the periods between upward zero crossings */
            if (bl[i - 1] < 0.0f && bl[i] >= 0.0f) {
                float at = (float)(i - 1u) + bl[i - 1] / (bl[i - 1] - bl[i]);
                if (nz++) {
                    float per = at - (float)last * 0.001f;
                    if (per < lo) lo = per;
                    if (per > hi) hi = per;
                }
                last = (uint32_t)(at * 1000.0f);
            }
        CHECK(hi - lo > 0.15f, "TAPE wow: the pitch wanders (periods %.2f .. %.2f samples)", lo, hi);
    }
}

/* the DJ filter is the break's: a low-passed break goes dark while the kick joining through the chain and the
 * drums around it stay bright; GLOUPI BREAKS: the kick's own (brk_fx_kick_dj) darkens the kick; FILTER THROW the
 * break */
static void test_dj_break(void)
{
    static float tone[BLOCK], z[BLOCK], k[BLOCK];
    brk_fx_t p;
    uint32_t i, b;
    float pk[5];
    int which;
    for (i = 0; i < BLOCK; i++) {
        tone[i] = 0.25f * sinf(6.2831853f * 4000.0f * (float)i / (float)BRK_SR);
        z[i] = 0.0f;
    }
    for (which = 0; which < 5; which++) {   /* the tone as the break, as thru, as post, as the kick, as the break with
                                             * FILTER THROW held */
        brk_fx_init();
        brk_fx_defaults(&p);
        p.dj = -1.0f;
        p.flt_target = -1.0f;
        p.flt_speed = 1.0f;
        brk_fx_set(&p);
        pk[which] = 0.0f;
        for (b = 0; b < 60u; b++) {
            if (which == 3) {
                for (i = 0; i < BLOCK; i++)
                    k[i] = tone[i];
                brk_fx_kick_dj(k, BLOCK, -1.0f);
                for (i = 0; i < BLOCK; i++)
                    bl[i] = k[i];
            } else
                brk_fx_process(which == 0 || which == 4 ? tone : z, which == 1 ? tone : 0, 0, 0, which == 2 ? tone : 0,
                               bl, br, BLOCK, which == 4 ? 1u << PAD_FILTER : 0u);
            if (b > 40u)
                for (i = 0; i < BLOCK; i++)
                    if (fabsf(bl[i]) > pk[which])
                        pk[which] = fabsf(bl[i]);
        }
    }
    CHECK(pk[0] < 0.01f && pk[1] > 0.2f && pk[2] > 0.2f && pk[3] < 0.01f,
          "DJ filter low-passed: the break %.3f, drums through the chain %.3f, around it %.3f; the drums' own: %.3f",
          pk[0], pk[1], pk[2], pk[3]);
    CHECK(pk[4] < 0.01f, "FILTER THROW held: the break low-passed (%.3f)", pk[4]);
}

/* LOAF BREAKER: the drums' filter: off at 0, a 40 Hz sub well down at 20 % (its high-pass), a 4 kHz tone well down at
 * -50 % (its low-pass) */
static void test_kick_hp(void)
{
    static float k[BLOCK];
    static const float POS[3] = {0.0f, 0.2f, -0.5f};
    float pk[3];
    uint32_t i, b, w;
    for (w = 0; w < 3u; w++) {
        pk[w] = 0.0f;
        for (b = 0; b < 200u; b++) {
            for (i = 0; i < BLOCK; i++)
                k[i] = 0.25f * sinf(6.2831853f * (w == 2u ? 4000.0f : 40.0f) * (float)(b * BLOCK + i) / (float)BRK_SR);
            brk_fx_kick_dj(k, BLOCK, POS[w]);
            if (b > 150u)
                for (i = 0; i < BLOCK; i++)
                    if (fabsf(k[i]) > pk[w])
                        pk[w] = fabsf(k[i]);
        }
    }
    CHECK(pk[0] > 0.24f && pk[1] < 0.1f && pk[2] < 0.1f,
          "DRUM FILTER: 40 Hz off %.3f, high-passed at 20 %% %.3f; 4 kHz low-passed at -50 %% %.3f", pk[0], pk[1], pk[2]);
    for (b = 0, pk[0] = 0.0f; b < 200u; b++) {   /* LOAF BREAKER: the same filter on the other drums */
        for (i = 0; i < BLOCK; i++)
            k[i] = 0.25f * sinf(6.2831853f * 40.0f * (float)(b * BLOCK + i) / (float)BRK_SR);
        brk_fx_drums_dj(0, k, BLOCK, 0.2f);
        if (b > 150u)
            for (i = 0; i < BLOCK; i++)
                if (fabsf(k[i]) > pk[0])
                    pk[0] = fabsf(k[i]);
    }
    CHECK(pk[0] < 0.1f, "DRUM FILTER: the snare, hat and clap high-passed too (40 Hz at 20 %%: %.3f)", pk[0]);
}

/* MULTIBAND: the bands sum back to the input untouched; a band pressed holds its level, the others don't move; CLIP:
 * clean below its edge, held at its ceiling above */
static void test_mb_clip(void)
{
    float gr[3], lv[3];
    float f60 = tone_peak(60.0f, FXT_MBCOMP, 0.0f, 0.0f, 0.0f, 1.0f, 8000u);
    float f1k = tone_peak(1000.0f, FXT_MBCOMP, 0.0f, 0.0f, 0.0f, 1.0f, 4000u);
    float f8k = tone_peak(8000.0f, FXT_MBCOMP, 0.0f, 0.0f, 0.0f, 1.0f, 4000u);
    float lo_q, lo_l, k1;
    CHECK(fabsf(f60 - 0.25f) < 0.01f && fabsf(f1k - 0.25f) < 0.01f && fabsf(f8k - 0.25f) < 0.01f,
          "MULTIBAND at 0: the bands sum back (60 Hz %.3f, 1 kHz %.3f, 8 kHz %.3f of 0.25)", f60, f1k, f8k);
    lo_q = tone_peak_a(60.0f, 0.05f, FXT_MBCOMP, 1.0f, 0.0f, 0.0f, 1.0f, 30000u);
    lo_l = tone_peak_a(60.0f, 0.25f, FXT_MBCOMP, 1.0f, 0.0f, 0.0f, 1.0f, 30000u);
    brk_fx_mb_meter(0u, gr, lv);
    CHECK(lo_l / lo_q < 1.5f && lo_l / lo_q > 1.0f, "MULTIBAND LOW: 14 dB more in, %.1f dB more out", 
          20.0f * log10f(lo_l / lo_q));
    CHECK(gr[0] < 0.4f && gr[1] > 0.95f && gr[2] > 0.95f, "MULTIBAND LOW: its reduction shown (%.2f %.2f %.2f)",
          gr[0], gr[1], gr[2]);
    k1 = tone_peak(1000.0f, FXT_MBCOMP, 1.0f, 0.0f, 0.0f, 1.0f, 4000u);
    CHECK(fabsf(k1 - 0.25f) < 0.025f, "MULTIBAND LOW pressed: 1 kHz untouched (%.3f)", k1);
    k1 = tone_peak(1000.0f, FXT_CLIP, 0.0f, 0.5f, 0.0f, 1.0f, 4000u);
    CHECK(fabsf(k1 - 0.25f) < 0.01f, "CLIP at 0 dB in: clean (%.3f)", k1);
    k1 = tone_peak(1000.0f, FXT_CLIP, 1.0f, 0.0f, 0.5f, 1.0f, 4000u);
    CHECK(k1 > 0.48f && k1 < 0.51f, "CLIP +24 dB into -6 dB: held at the ceiling (%.3f)", k1);
    k1 = tone_peak(1000.0f, FXT_CLIP, 1.0f, 1.0f, 0.5f, 1.0f, 4000u);
    CHECK(k1 > 0.48f && k1 < 0.51f, "CLIP hard: held at the ceiling too (%.3f)", k1);
}

/* slot 0 emptied for a block: its state (the levels followed) from nothing for the next run */
static void mb_fresh(void)
{
    static float z[BLOCK], zl[BLOCK], zr[BLOCK];
    brk_fx_t e;
    brk_fx_defaults(&e);
    brk_fx_slot_defaults(&e.slot[0], FXT_EMPTY);
    brk_fx_init();
    brk_fx_set(&e);
    brk_fx_process(z, 0, 0, 0, 0, zl, zr, BLOCK, 0u);
}

/* MB COMP's detail (PRD A-49a): a tone through slot 0's MB COMP with the band amounts a[] and the detail m; the peak
 * from `from` on (from < to), its meter afterwards */
static float mb_peak(float hz, float amp, const float *a, const brk_mbc_t *m, uint32_t from, uint32_t to,
                     float *gr, float *lvl)
{
    brk_fx_t p;
    uint32_t i, n = BRK_SR;
    float pk = 0.0f;
    for (i = 0; i < n; i++)
        buf[i] = amp * sinf(6.2831853f * hz * (float)i / (float)BRK_SR);
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], FXT_MBCOMP);
    p.slot[0].on = 1u;
    p.slot[0].p[0] = a[0];
    p.slot[0].p[1] = a[1];
    p.slot[0].p[2] = a[2];
    p.slot[0].mix = 1.0f;
    mb_fresh();
    brk_fx_set(&p);
    brk_fx_set_mbc(0u, m);
    for (i = 0; i < n; i += BLOCK)
        brk_fx_process(buf + i, 0, 0, 0, 0, bl + i, br + i, n - i < BLOCK ? n - i : BLOCK, 0u);
    for (i = from; i < to && i < n; i++)
        if (fabsf(bl[i]) > pk)
            pk = fabsf(bl[i]);
    if (gr && lvl)
        brk_fx_mb_meter(0u, gr, lvl);
    return pk;
}

static void test_mb_detail(void)
{
    static const float none[3] = {0.0f, 0.0f, 0.0f}, low[3] = {1.0f, 0.0f, 0.0f}, all[3] = {1.0f, 1.0f, 1.0f};
    brk_mbc_t m, d;
    float gr[3], lvl[3], a, b, q, l;
    brk_mbc_defaults(&d);
    /* the defaults are what MB COMP was: the old test's numbers through the new path */
    q = mb_peak(60.0f, 0.05f, low, &d, 30000u, BRK_SR, 0, 0);
    l = mb_peak(60.0f, 0.25f, low, &d, 30000u, BRK_SR, gr, lvl);
    CHECK(l / q < 1.5f && l / q > 1.0f && gr[0] < 0.4f && lvl[0] > 0.2f && lvl[2] < 0.01f,
          "MB detail defaults: 14 dB more in, %.1f dB out; low reduced to %.2f, its level %.2f, the high's %.3f",
          20.0f * log10f(l / q), gr[0], lvl[0], lvl[2]);
    m = d;                               /* the ratio at 1:1: nothing pressed whatever the amount */
    m.ratio[MB_LOW] = 0.0f;
    l = mb_peak(60.0f, 0.25f, low, &m, 30000u, BRK_SR, gr, lvl);
    CHECK(fabsf(l - 0.25f) < 0.015f && gr[0] > 0.999f, "MB 1:1: the low untouched at full amount (%.3f, gain %.3f)",
          l, gr[0]);
    mb_peak(60.0f, 0.25f, low, &d, 30000u, BRK_SR, gr, lvl);
    q = gr[0];                           /* LIMIT: pressed harder than 8:1 */
    m = d;
    m.ratio[MB_LOW] = 1.0f;
    mb_peak(60.0f, 0.25f, low, &m, 30000u, BRK_SR, gr, lvl);
    CHECK(gr[0] < q * 0.8f, "MB LIMIT: the low pressed harder than at 8:1 (gain %.3f, %.3f)", gr[0], q);
    m = d;                               /* a band's gain: +12 dB on the low, nothing pressed */
    m.gain[MB_LOW] = 1.0f;
    a = mb_peak(60.0f, 0.05f, none, &m, 8000u, BRK_SR, 0, 0);
    CHECK(a > 0.18f && a < 0.22f, "MB LOW GAIN +12 dB: 60 Hz four times (%.3f from 0.05)", a);
    m = d;                               /* the output: -12 dB */
    m.out = 0.0f;
    a = mb_peak(1000.0f, 0.2f, none, &m, 4000u, BRK_SR, 0, 0);
    CHECK(a > 0.045f && a < 0.055f, "MB OUTPUT -12 dB: a quarter (%.3f from 0.2)", a);
    m = d;                               /* the solo: the low alone takes 60 Hz, leaves 8 kHz out */
    m.solo = 1u + MB_LOW;
    a = mb_peak(60.0f, 0.25f, none, &m, 8000u, BRK_SR, 0, 0);
    b = mb_peak(8000.0f, 0.25f, none, &m, 4000u, BRK_SR, 0, 0);
    CHECK(a > 0.2f && b < 0.01f, "MB SOLO LOW: 60 Hz through (%.3f), 8 kHz gone (%.4f)", a, b);
    m.solo = 1u + MB_HIGH;
    a = mb_peak(60.0f, 0.25f, none, &m, 8000u, BRK_SR, 0, 0);
    b = mb_peak(8000.0f, 0.25f, none, &m, 4000u, BRK_SR, 0, 0);
    CHECK(a < 0.01f && b > 0.2f, "MB SOLO HIGH: 60 Hz gone (%.4f), 8 kHz through (%.3f)", a, b);
    m = d;                               /* the low split moved up to 800 Hz: the low band takes 500 Hz */
    m.solo = 1u + MB_LOW;
    a = mb_peak(500.0f, 0.25f, none, &m, 4000u, BRK_SR, 0, 0);
    m.xlo = 1.0f;
    b = mb_peak(500.0f, 0.25f, none, &m, 4000u, BRK_SR, 0, 0);
    CHECK(a < 0.08f && b > 0.15f, "MB LOW X 200 Hz -> 800 Hz: 500 Hz in the low band %.3f -> %.3f", a, b);
    m = d;                               /* the attack: slow lets the onset through, fast holds it */
    m.att[MB_LOW] = 1.0f;
    a = mb_peak(60.0f, 0.25f, low, &m, 0u, 1500u, 0, 0);
    m.att[MB_LOW] = 0.0f;
    b = mb_peak(60.0f, 0.25f, low, &m, 0u, 1500u, 0, 0);
    CHECK(a > b * 1.5f, "MB ATTACK 100 ms lets the first 34 ms through (%.3f), 0.1 ms holds it (%.3f)", a, b);
    m = d;                               /* the release: after the tone stops, a fast one lets go sooner */
    {
        brk_fx_t p;
        uint32_t i, r;
        float g_fast = 0.0f, g_slow = 0.0f;
        for (r = 0; r < 2u; r++) {
            m.rel[MB_LOW] = r ? 1.0f : 0.0f;
            for (i = 0; i < BRK_SR; i++)
                buf[i] = i < BRK_SR / 2u ? 0.25f * sinf(6.2831853f * 60.0f * (float)i / (float)BRK_SR) : 0.0f;
            brk_fx_defaults(&p);
            brk_fx_slot_defaults(&p.slot[0], FXT_MBCOMP);
            p.slot[0].on = 1u;
            p.slot[0].p[0] = 1.0f;
            mb_fresh();
            brk_fx_set(&p);
            brk_fx_set_mbc(0u, &m);
            for (i = 0; i < BRK_SR / 2u + 4410u; i += BLOCK)   /* the tone, then 100 ms of nothing */
                brk_fx_process(buf + i, 0, 0, 0, 0, bl + i, br + i, BLOCK, 0u);
            brk_fx_mb_meter(0u, gr, lvl);
            if (r)
                g_slow = gr[0];
            else
                g_fast = gr[0];
        }
        CHECK(g_fast > 0.9f && g_slow < 0.6f, "MB RELEASE: 100 ms after, 10 ms has let go (%.2f), 1 s not (%.2f)",
              g_fast, g_slow);
    }
    m = d;                               /* the knee: 4 dB under the threshold, hard does nothing, soft starts */
    a = 0.0316f * 0.63f / 0.85f;         /* (-30 dB, -4 dB, the mid band's share of 1 kHz) */
    mb_peak(1000.0f, a, all, &m, 4000u, BRK_SR, gr, lvl);
    q = gr[1];
    m.knee = 1.0f;
    mb_peak(1000.0f, a, all, &m, 4000u, BRK_SR, gr, lvl);
    CHECK(q > 0.999f && gr[1] < 0.995f, "MB KNEE: under the threshold, hard %.3f, soft 12 dB %.3f", q, gr[1]);
    CHECK(brk_mb_reduce(-1.0f, 0.5f, 0.0f) == 0.0f && fabsf(brk_mb_reduce(2.0f, 0.5f, 0.0f) - 1.0f) < 1e-6f &&
          fabsf(brk_mb_reduce(3.0f, 0.5f, 4.0f) - 1.5f) < 1e-6f && brk_mb_reduce(0.0f, 0.5f, 4.0f) > 0.0f,
          "MB curve: none under, slope above, the knee between");
}


/* the extras (PRD A-49b): slot 0 holds `type` at p[], mix, with the extras x (0: the effect's defaults); buf (n samples)
 * through it into bl, br */
static void fx1(int type, float p0, float p1, float p2, float mix, const brk_fxx_t *x, uint32_t n)
{
    brk_fx_t p;
    brk_fxx_t d;
    uint32_t i;
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], type);
    p.slot[0].on = 1u;
    p.slot[0].p[0] = p0;
    p.slot[0].p[1] = p1;
    p.slot[0].p[2] = p2;
    p.slot[0].mix = mix;
    mb_fresh();
    brk_fx_set(&p);
    brk_fxx_defaults(&d, type);
    brk_fx_set_fxx(0u, x ? x : &d);
    brk_fx_watch(0u);                    /* (slot 0's meters followed) */
    for (i = 0; i < n; i += BLOCK)
        brk_fx_process(buf + i, 0, 0, 0, 0, bl + i, br + i, n - i < BLOCK ? n - i : BLOCK, 0u);
}
static void tone_in(float hz, float amp, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        buf[i] = amp * sinf(6.2831853f * hz * (float)i / (float)BRK_SR);
}
static float peak_of(const float *b, uint32_t from, uint32_t to)
{
    float pk = 0.0f;
    uint32_t i;
    for (i = from; i < to; i++)
        if (fabsf(b[i]) > pk)
            pk = fabsf(b[i]);
    return pk;
}

static void test_fx_extras(void)
{
    uint32_t n = BRK_SR / 2u, t, j, v, i, ok = 1, bad_t = 0, bad_j = 0;
    brk_fxx_t x;
    brk_fx_meter_t m;
    float a, b;
    /* every effect, every extra at both ends, and every modulator shape at full depth: bounded, no NaN */
    for (i = 0; i < n; i++)              /* a loud, busy input: two tones and a click each beat */
        buf[i] = 0.5f * sinf(6.2831853f * 70.0f * (float)i / (float)BRK_SR) +
                 0.4f * sinf(6.2831853f * 3100.0f * (float)i / (float)BRK_SR) + (i % 11025u < 40u ? 0.6f : 0.0f);
    {
        static int16_t line[32768];      /* (the DELAY needs its line) */
        brk_fx_delay_buffer(line, 32768u);
    }
    for (t = 1; t < BRK_NFXT && ok; t++)
        for (j = 0; j < 4u + BRK_MOD_SHAPES && ok; j++)
            for (v = 0; v < 2u && ok; v++) {
                brk_fxx_defaults(&x, (int)t);
                if (j < 4u)
                    x.x[j] = (float)v;
                else {
                    x.x[FXX_SHAPE] = (float)(j - 4u) / (float)(BRK_MOD_SHAPES - 1u);
                    x.x[FXX_DEPTH] = (float)v;
                    x.x[FXX_RATE] = 1.0f;
                    x.x[FXX_TARGET] = (float)(j % 4u) / 3.0f;
                }
                fx1((int)t, 0.8f, 0.8f, 0.8f, 1.0f, &x, n);
                for (i = 0; i < n; i++)
                    if (!(fabsf(bl[i]) <= 1.0f) || !(fabsf(br[i]) <= 1.0f)) {
                        ok = 0;
                        bad_t = t;
                        bad_j = j;
                        break;
                    }
            }
    brk_fx_delay_buffer(0, 0u);
    CHECK(ok, "FX extras: every effect, every extra at both ends, every modulator: bounded (first bad: %s, %u)",
          ok ? "-" : BRK_FXT_NAME[bad_t], bad_j);
    /* FILTER 24 dB: a low-pass at 1 kHz takes more off 8 kHz than at 12 dB; SPREAD: the right brighter */
    tone_in(8000.0f, 0.25f, n);
    fx1(FXT_FILTER, 0.55f, 0.0f, 0.0f, 1.0f, 0, n);
    a = peak_of(bl, 8000u, n);
    brk_fxx_defaults(&x, FXT_FILTER);
    x.x[1] = 1.0f;
    fx1(FXT_FILTER, 0.55f, 0.0f, 0.0f, 1.0f, &x, n);
    b = peak_of(bl, 8000u, n);
    CHECK(b < a * 0.3f, "FILTER 24 dB: 8 kHz through a 1 kHz low-pass %.4f (12 dB %.4f)", b, a);
    brk_fxx_defaults(&x, FXT_FILTER);
    x.x[2] = 1.0f;
    tone_in(3000.0f, 0.25f, n);
    fx1(FXT_FILTER, 0.55f, 0.0f, 0.0f, 1.0f, &x, n);
    CHECK(peak_of(br, 8000u, n) > 2.0f * peak_of(bl, 8000u, n), "FILTER SPREAD: the right brighter (%.3f, left %.3f)",
          peak_of(br, 8000u, n), peak_of(bl, 8000u, n));
    /* EQ: the low shelf moved up to 400 Hz takes 300 Hz with it; LEVEL -12 dB a quarter */
    tone_in(300.0f, 0.05f, n);
    fx1(FXT_EQ, 1.0f, 0.5f, 0.5f, 0.5f, 0, n);
    a = peak_of(bl, 8000u, n);
    brk_fxx_defaults(&x, FXT_EQ);
    x.x[0] = 1.0f;
    fx1(FXT_EQ, 1.0f, 0.5f, 0.5f, 0.5f, &x, n);
    b = peak_of(bl, 8000u, n);
    CHECK(b > 1.6f * a, "EQ LOW FREQ 400 Hz: 300 Hz lifted %.3f (at 120 Hz %.3f)", b, a);
    brk_fxx_defaults(&x, FXT_EQ);
    x.x[3] = 0.0f;
    tone_in(1000.0f, 0.2f, n);
    fx1(FXT_EQ, 0.5f, 0.5f, 0.5f, 0.5f, &x, n);
    a = peak_of(bl, 4000u, n);
    CHECK(a > 0.045f && a < 0.055f, "EQ LEVEL -12 dB: a quarter (%.3f from 0.2)", a);
    /* CLIP OUTPUT -12 dB: the ceiling a quarter down */
    brk_fxx_defaults(&x, FXT_CLIP);
    x.x[1] = 0.0f;
    fx1(FXT_CLIP, 1.0f, 1.0f, 0.0f, 1.0f, &x, n);
    a = peak_of(bl, 4000u, n);
    CHECK(a > 0.24f && a < 0.26f, "CLIP OUTPUT -12 dB: held at a quarter (%.3f)", a);
    /* REVERB and DELAY WIDTH 0: the same both sides */
    for (i = 0; i < n; i++)
        buf[i] = i < 2000u ? 0.5f * sinf(6.2831853f * 440.0f * (float)i / (float)BRK_SR) : 0.0f;
    brk_fxx_defaults(&x, FXT_REVERB);
    x.x[0] = 0.0f;
    fx1(FXT_REVERB, 0.6f, 0.4f, 0.0f, 1.0f, &x, n);
    for (a = 0.0f, i = 3000u; i < n; i++)
        a += fabsf(bl[i] - br[i]);
    b = peak_of(bl, 3000u, n);
    CHECK(a < 1e-3f && b > 0.001f, "REVERB WIDTH 0: the tail rings (%.3f) the same left and right (%.5f apart)", b, a);
    brk_fxx_defaults(&x, FXT_DELAY);
    x.x[0] = 0.0f;
    {
        static int16_t line[32768];
        brk_fx_delay_buffer(line, 32768u);
        fx1(FXT_DELAY, 2.0f / 7.0f, 0.6f, 0.5f, 1.0f, &x, n);
        brk_fx_delay_buffer(0, 0u);
    }
    for (a = 0.0f, i = 3000u; i < n; i++)
        a += fabsf(bl[i] - br[i]);
    b = peak_of(bl, 3000u, n);
    CHECK(a < 1e-3f && b > 0.01f, "DELAY WIDTH 0: echoes (%.3f) the same left and right (%.5f apart)", b, a);
    /* PUNCH: 20:1 holds a loud tone nearer a quiet one than 1.5:1 */
    {
        float q[2], l[2];
        for (j = 0; j < 2u; j++) {
            brk_fxx_defaults(&x, FXT_PUNCH);
            x.x[0] = (float)j;
            tone_in(200.0f, 0.03f, n);
            fx1(FXT_PUNCH, 1.0f, 0.1f, 0.3f, 1.0f, &x, n);
            q[j] = peak_of(bl, 8000u, n);
            tone_in(200.0f, 0.3f, n);
            fx1(FXT_PUNCH, 1.0f, 0.1f, 0.3f, 1.0f, &x, n);
            l[j] = peak_of(bl, 8000u, n);
        }
        CHECK(l[1] / q[1] < 0.6f * l[0] / q[0], "PUNCH RATIO: 20 dB more in comes out %.1f dB louder at 20:1, %.1f at 1.5:1",
              20.0f * log10f(l[1] / q[1]), 20.0f * log10f(l[0] / q[0]));
    }
    /* MOD: a low-pass's cutoff swept by a sine at 1/16 makes a 2 kHz tone come and go; ENV follows the input */
    tone_in(2000.0f, 0.25f, BRK_SR);
    brk_fxx_defaults(&x, FXT_FILTER);
    x.x[FXX_DEPTH] = 1.0f;
    x.x[FXX_RATE] = 6.0f / 8.0f;
    fx1(FXT_FILTER, 0.5f, 0.0f, 0.0f, 1.0f, &x, BRK_SR);
    for (a = 1.0f, b = 0.0f, i = 0; i + 441u < BRK_SR; i += 441u) {   /* 10 ms windows: the quietest, the loudest */
        float w = i > 8000u ? peak_of(bl, i, i + 441u) : 0.5f;
        if (w < a) a = w;
        if (w > b) b = w;
    }
    CHECK(b > 0.15f && a < 0.03f, "MOD on FILTER's cutoff: 2 kHz comes and goes (%.3f .. %.3f)", a, b);
    brk_fxx_defaults(&x, FXT_DRIVE);
    x.x[FXX_SHAPE] = 1.0f;
    x.x[FXX_DEPTH] = 1.0f;
    tone_in(200.0f, 0.5f, n);
    fx1(FXT_DRIVE, 0.2f, 0.5f, 0.5f, 1.0f, &x, n);
    brk_fx_meter(0u, &m);
    a = m.mod;
    b = m.in;
    for (i = 0; i < n; i++)
        buf[i] = 0.0f;
    fx1(FXT_DRIVE, 0.2f, 0.5f, 0.5f, 1.0f, &x, n);
    brk_fx_meter(0u, &m);
    CHECK(a > 0.8f && b > 0.4f && m.mod < 0.05f && m.p[0] < 0.21f,
          "MOD ENV: loud %.2f (level in %.2f), silent %.2f; the drive back at %.2f", a, b, m.mod, m.p[0]);
}

/* the overload guard (PRD E-7): a render over 90 % bypasses the chain's last slot on; 2 s clear, it comes back
 * (LOAF BREAKER, 0.219: a lone block over sheds nothing, a second within 32 blocks does) */
static void test_overload(void)
{
    brk_fx_t p;
    uint32_t i, b, lone;
    float during, after, before;
    tone_in(8000.0f, 0.25f, BLOCK);
    brk_fx_defaults(&p);
    brk_fx_slot_defaults(&p.slot[0], FXT_FILTER);   /* a low-pass at 100 Hz: 8 kHz gone while it is on */
    p.slot[0].on = 1u;
    p.slot[0].p[0] = 0.2f;
    mb_fresh();
    brk_fx_set(&p);
    for (b = 0; b < 40u; b++)
        brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    before = peak_of(bl, 0u, BLOCK);
    brk_fx_overload();                   /* (a lone one: nothing shed) */
    brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    lone = brk_fx_shed();
    for (b = 0; b < 40u; b++)            /* (past its 32 blocks: forgotten) */
        brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    brk_fx_overload();
    brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    lone += brk_fx_shed();
    for (b = 0; b < 5u; b++)
        brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    brk_fx_overload();                   /* (a second within 32 blocks: the slot shed) */
    for (b = 0; b < 10u; b++)
        brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    during = peak_of(bl, 0u, BLOCK);
    i = brk_fx_shed();
    for (b = 0; b < 400u; b++)           /* 2.3 s */
        brk_fx_process(buf, 0, 0, 0, 0, bl, br, BLOCK, 0u);
    after = peak_of(bl, 0u, BLOCK);
    CHECK(lone == 0u && before < 0.01f && during > 0.2f && i == 1u && brk_fx_shed() == 0u && after < 0.01f,
          "OVERLOAD: a lone block sheds nothing (%u), two do (8 kHz %.3f -> %.3f, %u shed), back after 2 s (%.3f, %u)",
          lone, before, during, i, after, brk_fx_shed());
}

/* the snare and the hat (PRD K-10): each sounds on its own, in range, and dies away; the presets differ; a hit lands
 * on its sample; the snare's body is near its TUNE */
static void test_perc(void)
{
    static float o[BRK_SR / 2];
    perc_t p;
    uint32_t v, k, i, n = BRK_SR / 2, bounded = 1;
    perc_defaults(&p);
    for (v = 0; v < PV_STAB; v++)        /* (LOAF BREAKER: the snare, the hat, the clap; the MELODY's place empty) */
        for (k = 0; k < PERC_PRESETS; k++) {
            uint16_t at = 100u;
            uint8_t who = (uint8_t)(1u << v);
            float pk = 0.0f, tail = 0.0f, before = 0.0f;
            perc_preset(&p.v[v], v, k);
            perc_set(&p);
            perc_init();
            for (i = 0; i < n; i += BLOCK) {
                uint32_t m = n - i < BLOCK ? n - i : BLOCK;
                perc_render(o + i, m, &at, &who, i == 0 ? 1u : 0u);
            }
            for (i = 0; i < n; i++) {
                float a = fabsf(o[i]);
                if (i < 100u && a > before) before = a;
                if (a > pk) pk = a;
                if (i > n - 2205u && a > tail) tail = a;
            }
            if (!(pk > 0.1f && pk <= 1.0f && before == 0.0f && tail < pk * 0.2f)) {
                bounded = 0;
                printf("  perc %s %s: peak %.3f before %.4f tail %.3f\n", PERC_VOICE_NAME[v], PERC_PRESET_NAME[v][k], pk,
                       before, tail);
            }
            if (k == 0) {
                char name[24];
                snprintf(name, sizeof name, "perc_%s.wav", PERC_VOICE_NAME[v]);
                wav(name, o, o, n);
            }
        }
    CHECK(bounded, "snare and hat: every preset silent before its hit, in range, and gone in half a second");
    /* the snare's body alone (no noise, no drive) rings at its tune */
    perc_defaults(&p);
    p.v[PV_SNARE].snap = 0.0f;
    p.v[PV_SNARE].drive = 0.0f;
    p.v[PV_SNARE].decay = 1.0f;
    p.v[PV_SNARE].tune = 0.5f;
    p.v[PV_SNARE].tone = 0.0f;
    perc_set(&p);
    perc_init();
    perc_hit(PV_SNARE);
    for (i = 0; i < n; i += BLOCK)
        perc_render(o + i, n - i < BLOCK ? n - i : BLOCK, 0, 0, 0u);
    {                                    /* Goertzel: the energy at the tune against an octave and a half up */
        float want = perc_tune_hz(PV_SNARE, 0.5f), e[2];
        uint32_t f;
        for (f = 0; f < 2u; f++) {
            float w = 2.0f * cosf(6.2831853f * want * (f ? 2.9f : 1.0f) / (float)BRK_SR), a = 0.0f, b = 0.0f;
            for (i = 0; i < 8820u; i++) {
                float c = o[i] + w * a - b;
                b = a;
                a = c;
            }
            e[f] = a * a + b * b - w * a * b;
        }
        CHECK(e[0] > e[1] * 4.0f, "snare body at its tune (%.0f Hz): %.1f dB over 2.9x it", want,
              10.0f * log10f(e[0] / (e[1] + 1e-9f)));
    }
    {                                    /* the screen's shape of a hit: loud at its start, quiet by its end */
        perc_t ps;
        uint8_t px[66];
        uint32_t v2, c, pk;
        perc_defaults(&ps);
        for (v2 = 0; v2 < PV_STAB; v2++) {
            perc_shape(v2, &ps.v[v2], px, 66u, 400u, 30u);
            for (c = 0, pk = 0; c < 66u; c++)
                pk = px[c] > pk ? px[c] : pk;
            uint32_t at0 = 0u;
            for (c = 0; c < 66u; c++)
                if (px[c] == pk) {
                    at0 = c;
                    break;
                }
            CHECK(pk >= 3u && px[65] < pk / 2u && at0 <= 2u,
                  "perc_shape %u: a hit at its start, then its tail (%u .. %u)", v2, pk, px[65]);
        }
    }
}

/* the kick (PRD K-*): its layers, its decay, its hits on their samples, the presets, the steps' events */
static float kick_buf[BRK_SR];
static float kick_run(const kick_t *k, uint32_t n, uint32_t hit_at)
{
    uint32_t i = 0;
    float peak = 0.0f;
    kick_set(k);
    kick_init();
    while (i < n) {
        uint32_t m = n - i < BLOCK ? n - i : BLOCK, j;
        uint16_t at = (uint16_t)(hit_at - i);
        kick_render(kick_buf + i, m, &at, hit_at >= i && hit_at < i + m ? 1u : 0u);
        for (j = 0; j < m; j++)
            if (fabsf(kick_buf[i + j]) > peak)
                peak = fabsf(kick_buf[i + j]);
        i += m;
    }
    return peak;
}
static float span_peak(uint32_t a, uint32_t b)
{
    float p = 0.0f;
    for (; a < b; a++)
        if (fabsf(kick_buf[a]) > p)
            p = fabsf(kick_buf[a]);
    return p;
}

/* LOAF BREAKER: the KICK LAB: all 0 sounds as before; END cuts it; the TAIL FILTER darkens it; CLEAN keeps the tail
 * as loud; STACK plays another kick's attack, then this one */
static void test_kick_lab(void)
{
    kick_t k, a;
    kick_lab_t lab;
    static float ref[BRK_SR / 2];
    float pk, t0, t1;
    uint32_t i, zc0 = 0u, zc1 = 0u;
    int8_t syn[KS_N];
    memset(syn, 0, sizeof syn);
    memset(&lab, 0, sizeof lab);
    kick_set_synth(syn);
    kick_defaults(&k);
    KICK_TERROR(&k) = 60u;
    kick_set_lab(&lab);
    kick_set_stack(0, 0, 0);
    kick_run(&k, BRK_SR / 2, 0u);
    memcpy(ref, kick_buf, sizeof ref);
    lab.end = 9u;                        /* (200 ms) */
    kick_set_lab(&lab);
    pk = kick_run(&k, BRK_SR / 2, 0u);
    CHECK(span_peak(BRK_SR / 4, BRK_SR / 2) < 1e-3f && pk > 0.3f, "END cuts the kick at 200 ms (after: %.4f)",
          span_peak(BRK_SR / 4, BRK_SR / 2));
    CHECK(kick_lab_ms(KLM_END, 9u) > 199.0f && kick_lab_ms(KLM_END, 9u) < 201.0f &&
          kick_lab_ms(KLM_END, 49u) > 999.0f && kick_lab_ms(KLM_END, 49u) < 1001.0f &&
          kick_lab_ms(KLM_END, 50u) > 1049.0f && kick_lab_ms(KLM_END, 50u) < 1051.0f &&
          kick_lab_ms(KLM_END, KICK_END_MAX) > 2999.0f && kick_lab_ms(KLM_END, KICK_END_MAX) < 3001.0f,
          "END: 40 .. 1000 ms by 20 as saved before, then on to 3 s by 50 (0.9.70)");
    lab.end = 0u;
    lab.filt = 80u;
    lab.ftime = 30u;
    kick_set_lab(&lab);
    kick_run(&k, BRK_SR / 2, 0u);
    for (i = BRK_SR / 5; i < BRK_SR / 3; i++) {   /* (zero crossings: the brightness, 200 .. 333 ms) */
        zc0 += (ref[i - 1] < 0.0f) != (ref[i] < 0.0f);
        zc1 += (kick_buf[i - 1] < 0.0f) != (kick_buf[i] < 0.0f);
    }
    CHECK(zc1 * 2u < zc0, "the TAIL FILTER darkens the tail (%u crossings, %u without)", zc1, zc0);
    memset(&lab, 0, sizeof lab);
    lab.mode = 1u;
    kick_set_lab(&lab);
    kick_run(&k, BRK_SR / 2, 0u);
    t0 = span_peak(BRK_SR / 10, BRK_SR / 5);
    t1 = 0.0f;
    for (i = BRK_SR / 10; i < BRK_SR / 5; i++)
        t1 = fabsf(ref[i]) > t1 ? fabsf(ref[i]) : t1;
    CHECK(t0 > t1 * 0.7f && t0 < t1 * 1.4f, "CLEAN keeps the tail about as loud (%.2f, %.2f)", t0, t1);
    {                                    /* (0.9.71, FLEXOR) SPLIT: as CLASSIC up to its time, then a clean sine: as loud
                                          * there, far fewer crossings (the harmonics gone) */
        uint32_t sp, same = 1u, zr = 0u, zs = 0u;
        float bp, ap, rp;
        memset(&lab, 0, sizeof lab);
        lab.mode = KTM_SPLIT;
        lab.amt = 50u;
        kick_set_lab(&lab);
        kick_run(&k, BRK_SR / 2, 0u);
        sp = (uint32_t)(kick_lab_ms(KLM_SPLIT, 50u) * (float)(BRK_SR / 1000u));
        for (i = 0; i < sp; i++)
            same &= kick_buf[i] == ref[i];
        CHECK(same, "SPLIT: the body as CLASSIC, bit for bit, up to %u ms", (unsigned)(sp / (BRK_SR / 1000u)));
        bp = span_peak(sp - BRK_SR / 50u, sp);
        ap = span_peak(sp, sp + BRK_SR / 50u);
        CHECK(ap > bp * 0.6f && ap < bp * 1.4f, "SPLIT: no jump in level as the tail turns clean (%.2f, %.2f)", bp, ap);
        for (i = sp * 2u + 1u; i < BRK_SR / 2; i++) {
            zr += (ref[i - 1] < 0.0f) != (ref[i] < 0.0f);
            zs += (kick_buf[i - 1] < 0.0f) != (kick_buf[i] < 0.0f);
        }
        rp = span_peak(sp * 2u, BRK_SR / 2);
        CHECK(zs * 2u < zr + 2u && rp > 0.01f, "SPLIT: the tail a clean sine (%u crossings, %u as CLASSIC; %.3f)", zs, zr,
              rp);
    }
    {                                    /* DIRTY (on the 909, a light drive): the hit less driven, the tail harder */
        static float r9[BRK_SR / 2];
        float e0, e1, r0, r1;
        kick_preset(&a, 0u);
        a.volume = k.volume;
        memset(&lab, 0, sizeof lab);
        kick_set_lab(&lab);
        kick_run(&a, BRK_SR / 2, 0u);
        memcpy(r9, kick_buf, sizeof r9);
        lab.mode = KTM_DIRTY;
        lab.amt = 50u;
        kick_set_lab(&lab);
        kick_run(&a, BRK_SR / 2, 0u);
        e0 = span_peak(BRK_SR / 4, BRK_SR / 2);
        e1 = 0.0f;
        for (i = BRK_SR / 4; i < BRK_SR / 2; i++)
            e1 = fabsf(r9[i]) > e1 ? fabsf(r9[i]) : e1;
        for (i = 0, r0 = r1 = 0.0f; i < BRK_SR / 100; i++) {
            r0 += kick_buf[i] * kick_buf[i];
            r1 += r9[i] * r9[i];
        }
        CHECK(e0 > e1 * 1.1f && r0 < r1, "DIRTY: the tail dirtier and louder (%.3f, %.3f), the hit less driven (%.2f, %.2f)",
              e0, e1, r0, r1);
    }
    memset(&lab, 0, sizeof lab);
    lab.src = 1u;
    lab.cross = 50u;
    lab.fade = 40u;
    kick_preset(&a, 0u);                 /* (909's attack) */
    a.volume = k.volume;
    kick_set_stack(&a, syn, &lab);
    kick_set_lab(&lab);
    kick_set_solo(2u);                   /* (the tail alone: nothing before the hand-over) */
    kick_run(&k, BRK_SR / 2, 0u);
    CHECK(span_peak(0u, BRK_SR / 50) < 1e-6f && span_peak(BRK_SR / 10, BRK_SR / 5) > 0.1f,
          "STACK's SOLO TAIL: silent through the attack, then the tail");
    kick_set_solo(0u);
    pk = kick_run(&k, BRK_SR / 2, 0u);
    CHECK(pk > 0.1f && pk < 1.5f, "STACK sounds (%.2f)", pk);
    for (i = BRK_SR / 5; i < BRK_SR / 2 && fabsf(kick_buf[i] - ref[i]) < 1e-6f; i++)
        ;
    CHECK(i == BRK_SR / 2, "after the hand-over the kick is its own (from %u)", i);
    kick_set_stack(0, 0, 0);
    memset(&lab, 0, sizeof lab);
    kick_set_lab(&lab);
}

static void test_kick(void)
{
    kick_t k, solo;
    uint32_t i, l, n = BRK_SR / 2u, zc = 0;
    float p, first = 0.0f, prev_peak = -1.0f;
    int differ = 0, bounded = 1;
    kick_defaults(&k);
    CHECK(sizeof(kick_t) == 136u, "kick_t is %u bytes (136)", (unsigned)sizeof(kick_t));
    p = kick_run(&k, n, 1000u);
    CHECK(span_peak(0, 1000u) == 0.0f, "kick: silent before its hit (%.4f)", span_peak(0, 1000u));
    CHECK(p > 0.4f && p <= 1.0f, "kick: a full hit (peak %.3f)", p);
    wav("kick_default.wav", kick_buf, kick_buf, n);
    p = kick_run(&k, BRK_SR, 0u);
    CHECK(span_peak(BRK_SR - 2205u, BRK_SR) < p * 0.1f, "kick: it decays (%.3f at the end of a second, %.3f peak)",
          span_peak(BRK_SR - 2205u, BRK_SR), p);
    for (l = 0; l < KICK_LAYERS; l++) {  /* each layer alone sounds */
        uint32_t o;
        solo = k;
        solo.intensity = 0.0f;
        for (o = 0; o < KICK_LAYERS; o++)
            solo.l[o].level = o == l ? 1.0f : 0.0f;
        p = kick_run(&solo, n, 0u);
        CHECK(p > 0.05f && p <= 1.0f, "kick: %s alone sounds (peak %.3f)", KICK_LAYER_NAME[l], p);
        for (o = 0; o < KICK_FX; o++) {  /* and through each of its effects, bounded */
            solo.l[l].fx = (uint8_t)o;
            solo.l[l].fa = solo.l[l].fb = 0.8f;
            p = kick_run(&solo, n / 4u, 0u);
            if (!(p <= 1.0f) || (o != KFX_RING && p < 0.01f))
                bounded = 0;
        }
    }
    CHECK(bounded, "kick: every layer through every effect sounds and stays in range");
    /* SUB's pitch: 30 Hz at 0, 90 at 1; after its drop, the zero crossings say so */
    solo = k;
    solo.intensity = 0.0f;
    for (l = 0; l < KICK_LAYERS; l++)
        solo.l[l].level = l == KL_SUB ? 1.0f : 0.0f;
    solo.l[KL_SUB].pitch = 0.5f;
    solo.l[KL_SUB].decay = 1.0f;
    solo.l[KL_SUB].tone = 0.0f;
    kick_run(&solo, n, 0u);
    for (i = BRK_SR / 10u; i < BRK_SR / 10u * 4u; i++)
        if (kick_buf[i - 1] < 0.0f && kick_buf[i] >= 0.0f)
            zc++;
    {
        float hz = (float)zc / 0.3f, want = 30.0f * powf(2.0f, 0.5f * 1.585f);
        CHECK(fabsf(hz - want) < want * 0.1f, "kick: SUB at pitch 0.5 is %.1f Hz (%.1f)", hz, want);
    }
    /* decay longer, tail longer */
    solo.l[KL_SUB].decay = 0.1f;
    kick_run(&solo, n, 0u);
    first = span_peak(BRK_SR / 5u, BRK_SR / 5u + 2205u);
    solo.l[KL_SUB].decay = 0.9f;
    kick_run(&solo, n, 0u);
    CHECK(span_peak(BRK_SR / 5u, BRK_SR / 5u + 2205u) > first * 4.0f, "kick: DECAY lengthens the tail (%.3f vs %.3f)",
          span_peak(BRK_SR / 5u, BRK_SR / 5u + 2205u), first);
    /* INTENSITY: more drive, more of the tail held up near the peak */
    {
        float clean, hard;
        k.intensity = 0.0f;
        kick_run(&k, n, 0u);
        clean = span_peak(BRK_SR / 10u, BRK_SR / 10u + 2205u);
        k.intensity = 1.0f;
        kick_run(&k, n, 0u);
        hard = span_peak(BRK_SR / 10u, BRK_SR / 10u + 2205u);
        CHECK(hard > clean * 1.5f, "kick: INTENSITY drives the body up (%.3f vs clean %.3f)", hard, clean);
        kick_defaults(&k);
    }
    /* the presets: all bounded, all different */
    for (l = 0; l < KICK_PRESETS; l++) {
        float e = 0.0f;
        char name[40];
        kick_preset(&k, l);
        p = kick_run(&k, n, 0u);
        for (i = 0; i < n; i++)
            e += kick_buf[i] * kick_buf[i];
        if (!(p <= 1.0f && p > 0.3f))
            bounded = 0;
        if (prev_peak >= 0.0f && fabsf(e - prev_peak) > 1.0f)
            differ++;
        prev_peak = e;
        snprintf(name, sizeof name, "kick_%u.wav", l);
        wav(name, kick_buf, kick_buf, n);
    }
    CHECK(bounded && differ == (int)KICK_PRESETS - 1, "kick: %u presets, in range and each its own (%d differ)",
          KICK_PRESETS, differ);
    /* kick_hit (a MIDI note): at the next render */
    kick_defaults(&k);
    kick_set(&k);
    kick_init();
    kick_hit();
    kick_render(kick_buf, BLOCK, 0, 0u);
    CHECK(span_peak(0, BLOCK) > 0.3f, "kick: kick_hit sounds at the next render (%.3f)", span_peak(0, BLOCK));
    /* a retrigger mid-kick: the kick sounding fades out under the new one, no step (SUB and PUNCH start at 0) */
    {
        uint16_t at = 100u;
        solo = k;
        solo.intensity = 0.0f;
        solo.l[KL_GRIT].level = solo.l[KL_CLICK].level = 0.0f;
        kick_run(&solo, BRK_SR / 5u, 0u);    /* 200 ms in */
        kick_render(kick_buf, BLOCK, &at, 1u);
        p = fabsf(kick_buf[100] - kick_buf[99]);
        CHECK(p < 0.03f && fabsf(kick_buf[99]) > 0.05f, "kick: a retrigger doesn't click (step %.3f, at %.3f)", p,
              kick_buf[99]);
    }
    /* the shape for the screen: loud first, quiet at the end */
    {
        uint8_t px[60];
        kick_shape(&k, px, 60u, 1000u, 40u);
        CHECK(px[1] > 20u && px[59] < px[1] / 2u, "kick shape: %u at the start, %u at the end", px[1], px[59]);
    }
    /* the steps' events: 16 a bar, their places 0..15, a step apart */
    {
        uint16_t off[8];
        uint8_t idx[8];
        uint32_t sl = brk_step_len_samples(), t, ne, total = 0, bad = 0, last = 0;
        fresh(1);
        brk_play(1);
        for (t = 0; t < sl * 16u; t += BLOCK) {
            brk_render(buf, BLOCK);
            ne = brk_step_events(off, idx, 8u);
            for (i = 0; i < ne; i++) {
                uint32_t at = t + off[i];
                if (at >= sl * 16u)
                    break;
                if (idx[i] != (uint8_t)total || (total && (at - last + 2u < sl || at - last > sl + 2u)))
                    bad++;
                last = at;
                total++;
            }
        }
        CHECK(total == 16u && !bad, "step events: %u a bar (16), %u out of place", total, bad);
    }
}

/* the overlay effects (ARP): nothing on passes the sound through as it is; each one changes it and stays bounded;
 * REESE (LOAF BREAKER, was CUT) adds its bass, DIVE darkens over its sweep, RISE thins; let go, it fades back to the dry sound */
static int16_t ovl_buf[2u * OVL_LINE];
static float ovl_tone(uint32_t i, float hz) { return 0.4f * sinf(6.2831853f * hz * (float)i / (float)BRK_SR); }
static void ovl_run(uint32_t on, uint32_t from, uint32_t n, float hz, float *peak, float *diff)
{
    static uint8_t amt[OVL_N];
    uint32_t i, j;
    float l[BLOCK], r[BLOCK], spb = (float)BRK_SR * 60.0f / 120.0f;
    for (i = 0; i < OVL_N; i++)
        amt[i] = 100u;
    ovl_set(on, amt, 2u, 0u);
    *peak = 0.0f;
    *diff = 0.0f;
    for (i = from; i < from + n; i += BLOCK) {
        for (j = 0; j < BLOCK; j++)
            l[j] = r[j] = ovl_tone(i + j, hz) + 0.2f * ovl_tone(i + j, 97.0f);
        ovl_process(l, r, BLOCK, (float)i / spb, spb, 0xFFFFu);
        for (j = 0; j < BLOCK; j++) {
            float d = ovl_tone(i + j, hz) + 0.2f * ovl_tone(i + j, 97.0f);
            if (i + j >= from + n / 2u) {    /* the second half: past the fade and the sweep */
                if (fabsf(l[j]) > *peak) *peak = fabsf(l[j]);
                if (fabsf(r[j]) > *peak) *peak = fabsf(r[j]);
                *diff += fabsf(l[j] - d) + fabsf(r[j] - d);
            }
            if (!(l[j] == l[j]) || !(r[j] == r[j]))
                *peak = 99.0f;
        }
    }
    *diff /= (float)n;
}
static void test_ovl(void)
{
    uint32_t k;
    float pk, df, dry;
    ovl_init(ovl_buf);
    ovl_run(0u, 0u, 22050u, 2000.0f, &dry, &df);
    CHECK(df < 1e-6f && ovl_sounding() == 0u, "OVERLAY: nothing on, the sound passes as it is (%.6f off)", df);
    for (k = 0; k < OV_KX2; k++) {     /* (0.9.55: KICK X2, X4, TRI no sound here: the app's render) */
        ovl_init(ovl_buf);
        ovl_run(1u << k, 0u, 44100u, 2000.0f, &pk, &df);
        CHECK(pk < 1.5f, "OVERLAY %s: bounded (peak %.2f)", OVL_NAME[k], pk);
        CHECK(df > 0.01f, "OVERLAY %s: audible (%.4f off the dry sound)", OVL_NAME[k], df);
    }
    ovl_init(ovl_buf);
    ovl_run(7u << OV_KX2, 0u, 22050u, 2000.0f, &pk, &df);
    CHECK(df < 1e-6f, "OVERLAY KICK X2, X4, TRI: the sound as it is (%.6f off)", df);
    ovl_init(ovl_buf);
    ovl_run(1u << OV_DIVE, 0u, 88200u, 8000.0f, &pk, &df);
    CHECK(pk < 0.15f, "OVERLAY DIVE: an 8 kHz tone down past the sweep (peak %.3f of ~0.6)", pk);
    ovl_init(ovl_buf);
    ovl_run(1u << OV_RISE, 0u, 88200u, 97.0f, &pk, &df);
    CHECK(pk < 0.1f, "OVERLAY RISE: the low end gone past the sweep (peak %.3f)", pk);
    ovl_run(0u, 88200u, 22050u, 2000.0f, &pk, &df);   /* let go: back to dry (the filter's tail aside) */
    CHECK(df < 0.002f && ovl_sounding() == 0u, "OVERLAY: let go, back to the dry sound (%.4f off)", df);
}

/* LOAF BREAKER: the VINYL BREAK. A gated slice gone quiet comes back and slows (the stop no longer cuts off), and the
 * kick at half speed has half its cycles */
static void test_vinyl(void)
{
    static brk_break_t tone;
    static float sp[BLOCK];
    brk_padset_t ps;
    kick_t k;
    uint32_t i, n = 22050u, s, m, zc[2];
    float before, after;
    for (i = 0; i < n; i++)
        pcm[i] = (int16_t)(12000.0f * sinf(6.2831853f * (float)i * (500.0f / 22050.0f)));
    brk_init();
    brk_padset_defaults(&ps);
    brk_padset(&ps);
    tone.pcm = pcm;
    tone.len = n;
    tone.rate = 22050u;
    tone.bpm_x100 = 24000u;
    tone.bars = 1u;
    tone.nslices = 16u;
    for (i = 0; i <= 16u; i++)
        tone.slice[i] = i * n / 16u;
    brk_set_break(&tone);
    brk_tempo(240.0f);
    brk_pitch(0.0f);
    brk_gate(0.25f);
    brk_play(1);
    s = brk_step_len_samples();
    render(buf, 2u * s + s / 2u, 0, 0, 0);   /* half a step in: the gated slice quiet */
    before = render(buf, BLOCK, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 1);
    render(buf, BLOCK, 0, 0, 0);
    after = render(buf, BLOCK, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 0);
    brk_play(0);
    brk_gate(1.0f);
    CHECK(before < 0.02f && after > 0.1f, "VINYL BREAK: a gated slice plays on as it slows (%.3f, then %.3f)", before,
          after);
    ps.stop_beats = 16.0f;               /* 16 beats: still turning after 8, stopped after 16 */
    brk_padset(&ps);
    brk_play(1);
    render(buf, 2u * s, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 1);
    render(buf, 32u * s, 0, 0, 0);
    before = render(buf, BLOCK, 0, 0, 0);
    render(buf, 32u * s, 0, 0, 0);
    after = render(buf, BLOCK, 0, 0, 0);
    brk_pad(PAD_TAPESTOP, 0);
    brk_play(0);
    brk_padset_defaults(&ps);
    brk_padset(&ps);
    CHECK(before > 0.05f && after < 0.01f, "VINYL BREAK: 16 beats long (%.3f at 8, %.3f at 16)", before, after);
    for (m = 0; m < 2u; m++) {
        uint16_t at = 0u;
        kick_defaults(&k);
        k.intensity = 0.0f;
        k.l[KL_GRIT].level = k.l[KL_CLICK].level = 0.0f;
        kick_set(&k);
        kick_init();
        for (i = 0; i < BLOCK; i++)
            sp[i] = 0.5f;
        for (i = 0; i < 35u * BLOCK; i += BLOCK) {
            kick_speed(m ? sp : 0);
            kick_render(kick_buf + i, BLOCK, &at, i == 0u ? 1u : 0u);
        }
        kick_speed(0);
        for (i = 1, zc[m] = 0; i < 35u * BLOCK; i++)
            if ((kick_buf[i - 1] < 0.0f) != (kick_buf[i] < 0.0f))
                zc[m]++;
    }
    CHECK(zc[1] * 10u > zc[0] * 3u && zc[1] * 10u < zc[0] * 7u, "VINYL BREAK: the kick at half speed (%u cycles, %u)",
          zc[0], zc[1]);
}

/* LOAF BREAKER: brk_rewind: at the next step (on time), the pattern's first: step 0, the drums' first bar */
static void test_rewind(void)
{
    brk_status_t st;
    uint32_t s;
    fresh(2);
    brk_play(1);
    s = brk_step_len_samples();
    render(buf, s * 21u + s / 2u, 0, 0, 0);   /* step 21 playing */
    brk_rewind();
    render(buf, s, 0, 0, 0);
    brk_status(&st);
    brk_play(0);
    CHECK(st.step == 0 && st.bar64 == 0 && st.bar16 == 0, "REWIND: the next step is the pattern's first (step %d, bar %d / %d)",
          (int)st.step, (int)st.bar16, (int)st.bar64);
}

/* LOAF BREAKER (crossbreed): a kick hit at a note an octave up (kick_pitches 2) has about twice the cycles */
static void test_kick_notes(void)
{
    kick_t k;
    float pm = 2.0f;
    uint32_t m, i, zc[2];
    for (m = 0; m < 2u; m++) {
        uint16_t at = 0u;
        kick_defaults(&k);
        k.intensity = 0.0f;
        k.l[KL_GRIT].level = k.l[KL_CLICK].level = 0.0f;
        kick_set(&k);
        kick_init();
        for (i = 0; i < 35u * BLOCK; i += BLOCK) {
            kick_pitches(m ? &pm : 0);
            kick_render(kick_buf + i, BLOCK, &at, i == 0u ? 1u : 0u);
        }
        kick_pitches(0);
        for (i = 1, zc[m] = 0; i < 35u * BLOCK; i++)
            if ((kick_buf[i - 1] < 0.0f) != (kick_buf[i] < 0.0f))
                zc[m]++;
    }
    CHECK(zc[1] * 10u > zc[0] * 15u && zc[1] * 10u < zc[0] * 25u, "KICK NOTE: an octave up, about twice the cycles (%u, %u)",
          zc[0], zc[1]);
}

/* LOAF BREAKER: TERROR 2 changes the kick and stays in range */
static void test_terror2(void)
{
    kick_t k;
    float pk[2], df = 0.0f;
    static float dry[35u * BLOCK];
    uint32_t m, i;
    for (m = 0; m < 2u; m++) {
        uint16_t at = 0u;
        kick_preset(&k, 0u);
        k.volume = 0.8f;
        KICK_TERROR2(&k) = m ? 100u : 0u;
        kick_set(&k);
        kick_init();
        for (i = 0; i < 35u * BLOCK; i += BLOCK)
            kick_render(kick_buf + i, BLOCK, &at, i == 0u ? 1u : 0u);
        for (i = 0, pk[m] = 0.0f; i < 35u * BLOCK; i++) {
            if (fabsf(kick_buf[i]) > pk[m]) pk[m] = fabsf(kick_buf[i]);
            if (!m) dry[i] = kick_buf[i];
            else df += fabsf(kick_buf[i] - dry[i]);
        }
    }
    df /= (float)(35u * BLOCK);
    CHECK(pk[1] <= 1.0f && pk[1] > 0.3f && df > 0.05f, "TERROR 2: bounded (%.3f) and far from the dry kick (%.3f)", pk[1], df);
}

/* LOAF BREAKER: the kick's SYNTH: HOLD keeps it up longer, RUMBLE leaves a tail after it, TUNE moves it, all bounded */
static int16_t rumble_line[4096];
static float kick_energy(const int8_t *syn, uint32_t from, uint32_t to, float *pk)
{
    kick_t k;
    uint16_t at = 0u;
    uint32_t i;
    float e = 0.0f;
    kick_preset(&k, 3u);
    k.volume = 0.8f;
    kick_rumble_buf(rumble_line, 4096u);
    kick_set_synth(syn);
    kick_set(&k);
    kick_init();
    for (i = 0; i < 60u * BLOCK; i += BLOCK)
        kick_render(kick_buf + i, BLOCK, &at, i == 0u ? 1u : 0u);
    for (i = 0; i < 60u * BLOCK; i++) {
        if (i >= from && i < to)
            e += kick_buf[i] * kick_buf[i];
        if (fabsf(kick_buf[i]) > *pk)
            *pk = fabsf(kick_buf[i]);
    }
    return e;
}
static void test_kick_synth(void)
{
    int8_t s[KS_N] = {0};
    float pk = 0.0f, base_tail, hold_tail, rum_tail, tuned;
    uint32_t a = 20u * BLOCK, b = 60u * BLOCK;
    base_tail = kick_energy(s, a, b, &pk);
    s[KS_HOLD] = 40;
    hold_tail = kick_energy(s, a, b, &pk);
    s[KS_HOLD] = 0;
    s[KS_RUMBLE] = 50;
    rum_tail = kick_energy(s, a, b, &pk);
    s[KS_RUMBLE] = 0;
    s[KS_TUNE] = 12;
    tuned = kick_energy(s, 0u, 4u * BLOCK, &pk);
    s[KS_TUNE] = 0;
    CHECK(hold_tail > base_tail * 1.2f && rum_tail > base_tail * 1.2f && tuned > 0.0f && pk <= 1.3f,
          "KICK SYNTH: HOLD %.3f and RUMBLE %.3f against %.3f after 120 ms, peak %.2f", hold_tail, rum_tail, base_tail, pk);
    kick_set_synth(s);
}

/* LOAF BREAKER TEST: the REVERSE BASS swells between two kicks (the beat's end louder than without it, bounded), and
 * GLIDE slides the second kick's pitch from the first's (more crossings just after it than a jump down) */
static uint32_t kick_crossings(float glide, uint32_t from, uint32_t to)
{
    kick_t k;
    uint16_t at = 0u;
    uint32_t i, n = 0u;
    int8_t s[KS_N] = {0};
    float pm1 = 1.0f, pm2 = 0.5f;
    kick_preset(&k, 3u);
    k.volume = 0.8f;
    kick_set_synth(s);
    kick_set(&k);
    kick_set_bass(glide, 0.0f, 0.5f, 0.5f);
    kick_init();
    for (i = 0; i < 60u * BLOCK; i += BLOCK) {
        kick_pitches(i == 0u ? &pm1 : &pm2);
        kick_render(kick_buf + i, BLOCK, &at, i == 0u || i == 20u * BLOCK ? 1u : 0u);
    }
    kick_pitches(0);
    {                                    /* (the sub's: low-passed, the click and the grit left out) */
        float lp = 0.0f, prev = 0.0f;
        for (i = from; i < to; i++) {
            lp += (kick_buf[i] - lp) * 0.04f;
            if (i > from)
                n += (prev < 0.0f) != (lp < 0.0f);
            prev = lp;
        }
    }
    return n;
}
static void test_kick_bass(void)
{
    int8_t s[KS_N] = {0};
    float pk = 0.0f, base, rb;
    uint32_t spb = 50u * BLOCK, cj, cg;
    s[KS_HOLD] = -50;                    /* (a short kick: the beat's end its own) */
    kick_set_beat((float)spb);
    kick_set_bass(0.0f, 0.0f, 0.5f, 0.5f);
    base = kick_energy(s, spb * 3u / 4u, spb - 100u, &pk);
    kick_set_bass(0.0f, 1.0f, 0.5f, 0.5f);
    rb = kick_energy(s, spb * 3u / 4u, spb - 100u, &pk);
    kick_set_bass(0.0f, 0.0f, 0.5f, 0.5f);
    CHECK(rb > base * 4.0f + 1.0f && pk <= 1.3f, "REVERSE BASS: %.2f at the beat's end against %.2f, peak %.2f", rb, base, pk);
    cj = kick_crossings(0.0f, 20u * BLOCK + 300u, 20u * BLOCK + 4000u);
    cg = kick_crossings(150.0f, 20u * BLOCK + 300u, 20u * BLOCK + 4000u);
    kick_set_bass(0.0f, 0.0f, 0.5f, 0.5f);
    CHECK(cg > cj, "GLIDE: %u crossings after the second kick sliding, %u jumping", cg, cj);
}

/* LOAF BREAKER (TEST): TERROR's more: CLIP and CRUSH sound apart from FOLD, SCREECH and DRIVE move it, all bounded */
static void test_terror_more(void)
{
    kick_t k;
    static float ref[35u * BLOCK];
    float df[4], pk = 0.0f;
    uint32_t m, i;
    for (m = 0; m < 5u; m++) {
        uint16_t at = 0u;
        kick_preset(&k, 0u);
        k.volume = 0.8f;
        KICK_TERROR(&k) = 60u;
        KICK_T_TYPE(&k) = (uint8_t)(m == 1u ? 1u : m == 2u ? 2u : 0u);
        KICK_T_SCREECH(&k) = (uint8_t)(m == 3u ? 60u : 0u);
        KICK_T_DRIVE(&k) = (uint8_t)(int8_t)(m == 4u ? 40 : 0);
        kick_set(&k);
        kick_init();
        for (i = 0; i < 35u * BLOCK; i += BLOCK)
            kick_render(kick_buf + i, BLOCK, &at, i == 0u ? 1u : 0u);
        if (m)
            df[m - 1u] = 0.0f;
        for (i = 0; i < 35u * BLOCK; i++) {
            if (!m)
                ref[i] = kick_buf[i];
            else
                df[m - 1u] += fabsf(kick_buf[i] - ref[i]);
            if (fabsf(kick_buf[i]) > pk)
                pk = fabsf(kick_buf[i]);
        }
    }
    for (m = 0; m < 4u; m++)
        df[m] /= (float)(35u * BLOCK);
    CHECK(pk <= 1.2f && df[0] > 0.005f && df[1] > 0.005f && df[2] > 0.005f && df[3] > 0.005f,
          "TERROR +: CLIP %.3f, CRUSH %.3f, SCREECH %.3f, DRIVE %.3f apart from the plain fold, peak %.2f", df[0], df[1],
          df[2], df[3], pk);
}

/* LOAF BREAKER: KICK FX: the kick's own slots on its buffer (in place), the chain's apart. A RING there moves the
 * kick; a REVERB there (not allowed) passes it on untouched; bypassed, untouched */
static void test_kick_fx(void)
{
    static float x[BLOCK], y[BLOCK];
    brk_fxslot_t s;
    brk_fxx_t xx;
    uint32_t m, i, b;
    float df[3];
    brk_fx_init();
    for (m = 0; m < 3u; m++) {
        brk_fx_slot_defaults(&s, m == 1u ? FXT_REVERB : FXT_RING);
        s.on = (uint8_t)(m != 2u);
        s.mix = 1.0f;
        brk_fxx_defaults(&xx, s.type);
        brk_fx_set_kick_slot(0u, &s);
        brk_fx_set_fxx(BRK_FX_SLOTS, &xx);
        df[m] = 0.0f;
        for (b = 0; b < 8u; b++) {
            for (i = 0; i < BLOCK; i++)
                x[i] = y[i] = 0.5f * sinf(6.2831853f * 60.0f * (float)(b * BLOCK + i) / (float)BRK_SR);
            brk_fx_kick(x, BLOCK);
            for (i = 0; i < BLOCK; i++)
                df[m] += fabsf(x[i] - y[i]);
        }
        df[m] /= (float)(8u * BLOCK);
    }
    CHECK(df[0] > 0.05f && df[1] < 1e-6f && df[2] < 1e-6f, "KICK FX: a RING moves the kick (%.3f), a REVERB (%.6f) and a bypass (%.6f) don't",
          df[0], df[1], df[2]);
    brk_fx_slot_defaults(&s, FXT_EMPTY);
    s.on = 0u;
    brk_fx_set_kick_slot(0u, &s);
}

/* LOAF BREAKER (0.256): BREAK TIME HALF: at twice the break's tempo, its steps every other 16th; the drums' steps
 * (brk_step_events) and the clock out at the tempo, 16 and 96 a bar */
static void test_half_time(void)
{
    uint16_t off[64];
    uint8_t idx[64];
    uint32_t sl, t, ev = 0, clk = 0, bsteps = 0;
    int32_t last = -2;
    brk_status_t st;
    fresh(2);
    brk_tempo(BPM * 2.0f);
    brk_half_time(1);
    brk_play(1);
    sl = brk_step_len_samples();
    for (t = 0; t < sl * 16u; t += 64u) {   /* a bar of the tempo */
        uint32_t k, n;
        brk_render(buf, 64);
        n = brk_step_events(off, idx, 64u);
        for (k = 0; k < n; k++)
            ev += t + off[k] < sl * 16u;     /* (this bar's, not the next one's first) */
        n = brk_clock_ticks(off, 64u);
        for (k = 0; k < n; k++)
            clk += t + off[k] < sl * 16u;
        brk_status(&st);
        if (st.step != last) {
            last = st.step;
            bsteps++;
        }
    }
    CHECK(ev == 16u, "half time: the drums' steps %u a bar (16)", ev);
    CHECK(clk == 96u, "half time: the clock %u ticks a bar (96)", clk);
    CHECK(bsteps >= 8u && bsteps <= 9u, "half time: the break's steps %u a bar of the tempo (8)", bsteps);
    brk_half_time(0);
}

/* LOAF BREAKER (0.9.67): the drums' filter's kinds: SMOOTH and SOFT gentler than CLASSIC at the same knob, RESO a peak */
static float dft_gain(uint32_t t, float pos, float hz)
{
    static float x[256], y[256];
    float ph = 0.0f, in = 0.0f, out = 0.0f;
    int b, i;
    brk_fx_drum_type(t);
    for (b = 0; b < 300; b++) {
        for (i = 0; i < 256; i++) {
            x[i] = y[i] = sinf(ph);
            ph += 6.2831853f * hz / 44100.0f;
            if (ph > 6.2831853f)
                ph -= 6.2831853f;
        }
        brk_fx_drums_dj(y, 0, 256, pos);
        if (b > 150)
            for (i = 0; i < 256; i++) {
                in += x[i] * x[i];
                out += y[i] * y[i];
            }
    }
    return 10.0f * log10f(out / in);
}
static void test_drum_filter_kinds(void)
{
    float c = dft_gain(BRK_DFT_CLASSIC, 0.5f, 200.0f), m = dft_gain(BRK_DFT_SMOOTH, 0.5f, 200.0f);
    float so = dft_gain(BRK_DFT_SOFT, 0.5f, 200.0f), r = dft_gain(BRK_DFT_RESO, -0.5f, 1000.0f);
    CHECK(c < -12.0f && m > -4.0f && so > -4.0f, "drum filter at half: CLASSIC %.1f dB, SMOOTH %.1f, SOFT %.1f at 200 Hz", c, m, so);
    CHECK(r > 4.0f, "RESO's peak: %.1f dB", r);
    CHECK(brk_fx_drum_hz(0.5f, BRK_DFT_CLASSIC) == 50.0f * brk_exp2f(3.5f), "CLASSIC's cutoff as it always was");
    brk_fx_drum_type(BRK_DFT_CLASSIC);
}
/* K-GLITCH 0.2b: the CHOP lane's triggers (brk_kg_cond): a condition, a chance, the density */
static void kg_run_lane(const uint8_t *lane, const uint8_t *cnd, uint32_t den, int32_t *sls, int32_t *seen)
{
    uint32_t sl;
    fresh(2);
    brk_kg(0u, lane, 16u);
    brk_kg_cond(cnd, den);
    sl = brk_step_len_samples();
    brk_play(1);
    render(buf, sl * 64u + sl / 2u, seen, sls, 64);
    brk_kg(0u, 0, 0u);
    brk_kg_cond(0, 100u);
}
static int kg_gaps(const int32_t *a, int from, int to)   /* steps with no new hit (SKIP fired): the slice held */
{
    int i, n = 0;
    for (i = from; i < to; i++)
        n += a[i] < 0 || a[i] == a[i - 1];
    return n;
}
static void test_kg_cond(void)
{
    static uint8_t lane[16], cnd[16];
    int32_t q[64], n, i, ok = 0, g;
    memset(lane, KGC_SKIP | 3u << 6, sizeof lane);   /* (every step SKIP at 100 %: no hit) */
    memset(cnd, KGQ_ALWAYS, sizeof cnd);
    kg_run_lane(lane, cnd, 100u, q, &n);
    g = kg_gaps(q, 1, 64);
    CHECK(n >= 60 && g >= 60, "ALWAYS: SKIP on every step (%d of 63 without a hit)", g);
    kg_run_lane(lane, cnd, 0u, q, &n);
    g = kg_gaps(q, 1, 64);
    CHECK(g <= 6, "DENSITY 0: nothing fires (%d of 63 without a hit)", g);
    memset(cnd, KGQ_1_2, sizeof cnd);
    kg_run_lane(lane, cnd, 100u, q, &n);
    for (i = 0; i < 4; i++) {
        g = kg_gaps(q, i * 16 + 1, i * 16 + 16);
        ok += i & 1 ? g <= 3 : g >= 14;
    }
    CHECK(ok == 4, "1:2: rounds 1, 3 with the effect, 2, 4 without (%d of 4 right)", ok);
    memset(cnd, KGQ_3_4, sizeof cnd);
    kg_run_lane(lane, cnd, 100u, q, &n);
    for (ok = 0, i = 0; i < 4; i++) {
        g = kg_gaps(q, i * 16 + 1, i * 16 + 16);
        ok += i == 2 ? g >= 14 : g <= 3;
    }
    CHECK(ok == 4, "3:4: the third round alone (%d of 4 right)", ok);
    memset(cnd, KGQ_ALWAYS | 8u << 4, sizeof cnd);   /* (CHANCE 50 %) */
    kg_run_lane(lane, cnd, 100u, q, &n);
    g = kg_gaps(q, 1, 64);
    CHECK(g >= 18 && g <= 45, "CHANCE 50 %%: about half fire (%d of 63)", g);
    memset(cnd, KGQ_ALWAYS, sizeof cnd);
    kg_run_lane(lane, cnd, 50u, q, &n);
    g = kg_gaps(q, 1, 64);
    CHECK(g >= 18 && g <= 45, "DENSITY 50 %%: about half fire (%d of 63)", g);
    memset(cnd, KGQ_FIRST, sizeof cnd);
    kg_run_lane(lane, cnd, 100u, q, &n);
    CHECK(kg_gaps(q, 1, 16) >= 14 && kg_gaps(q, 17, 64) <= 6, "FIRST: the first round only");
    memset(cnd, KGQ_NFIRST, sizeof cnd);
    kg_run_lane(lane, cnd, 100u, q, &n);
    CHECK(kg_gaps(q, 1, 16) <= 3 && kg_gaps(q, 17, 64) >= 44, "NOT FIRST: every round but the first");
}
int main(int argc, char **argv)
{
    if (argc > 1)
        outdir = argv[1];
    test_straight();
    test_tempo_accuracy();
    test_free_play();
    test_quantised_key();
    test_hold_loops();
    test_split();
    test_chaos();
    test_kg_cond();
    test_key_loops();
    test_clock();
    test_keep_length();
    test_half_master();
    test_half_time();
    test_vinyl();
    test_kick_hp();
    test_rewind();
    test_terror2();
    test_kick_notes();
    test_kick_fx();
    test_terror_more();
    test_kick_synth();
    test_kick_bass();
    test_gate();
    test_swap();
    test_snap();
    test_reverb();
    test_stop();
    test_delay_slices();
    test_eq_tape();
    test_mb_clip();
    test_mb_detail();
    test_fx_extras();
    test_overload();
    test_dj_break();
    test_kick();
    test_kick_lab();
    test_perc();
    test_generator();
    test_pads();
    test_fx();
    test_adpcm();
    test_key_decay();
    test_perc_sample();
    test_pad_settings();
    test_dub();
    test_styles();
    test_ovl();
    test_drum_filter_kinds();
    printf("%d of %d checks passed\n", checks - fails, checks);
    return fails ? 1 : 0;
}
