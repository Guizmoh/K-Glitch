/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's snare and hi-hat (perc.h). */
#define BRK_ENGINE 1                     /* (LOAF BREAKER, 0.226: the maths inlined into the per-sample code) */
#include "perc.h"
#include "brk.h"
#include "brk_math.h"
#include "adpcm.h"

const char *const PERC_VOICE_NAME[PERC_VOICES] = {"SNARE", "HAT", "CLAP", ""};   /* (LOAF BREAKER, 0.209: the fourth,
                                          * the MELODY's, gone: kicks and drums only; its place kept, the saves' layout) */
const char *const PERC_PRESET_NAME[PERC_VOICES][PERC_PRESETS] = {{"909", "TIGHT", "FAT", "GABBER"},
                                                                 {"CLOSED", "808", "OPEN", "TRASH"},
                                                                 {"808", "909", "TIGHT", "BIG"},
                                                                 {"", "", "", ""}};

static inline float soft(float x) { return x / (1.0f + brk_absf(x)); }
const char *const DIRT_NAME[DIRT_N] = {"SOFT", "HARD", "FOLD", "BITS"};
static float decay_factor(float ms) { return brk_exp2f_c(-6.644f / (ms * 0.001f * (float)BRK_SR)); }   /* -40 dB */

static float onepole(float hz) { return 1.0f - brk_exp2f_c(-9.0647f * hz / (float)BRK_SR); }   /* 1 - e^(-2 pi f / fs) */

static const float HAT_RATIO[6] = {1.0f, 1.4827f, 1.8003f, 2.5460f, 2.6303f, 3.8967f};   /* 808: 205.3 .. 800 Hz */

BRK_COLD float perc_tune_hz(uint32_t voice, float t)
{
    t = brk_clampf(t, 0.0f, 1.0f);
    return voice == PV_SNARE ? 120.0f * brk_exp2f_c(t * 1.737f) : voice == PV_CLAP ? 700.0f * brk_exp2f_c(t * 1.8f)
           : 100.0f * brk_exp2f_c(t * 3.0f);
}
BRK_COLD float perc_decay_ms(uint32_t voice, float d)
{
    d = brk_clampf(d, 0.0f, 1.0f);
    return voice == PV_SNARE ? 60.0f + 640.0f * d * d : voice == PV_CLAP ? 60.0f + 840.0f * d * d
           : 15.0f + 885.0f * d * d;
}

typedef struct {                         /* a voice sounding */
    float env, env2, pe, ph[6], hp1, hp2, lp, last, declick;
    float lenv, lg, bits;                /* (LOAF BREAKER: LIFT's level followed and its gain; BITS' sample held) */
    uint32_t ln;
    adpcm_state_t sst;                   /* (LOAF BREAKER 0.9.45: its sample: the decoder, the next sample to decode, */
    uint32_t spos;                       /* the two read between, how far between; playing) */
    float sa, sb, sf;
    uint8_t sact;
} pv_t;
typedef struct {                         /* a voice's settings, mapped for the block */
    float f, dec, dec2, lvl, hpk, lpk, body, noise, crack, g, make;
    float depth, kpe, lift, room;        /* (LOAF BREAKER: the drop, its fall a sample, LIFT, the room's send) */
    uint32_t dirt, gate;                 /* (the saturation; the room's length in samples) */
    const uint8_t *sdat;                 /* (0.9.45: its sample, or 0: the synthesis; its length; its step a sample at */
    uint32_t slen;                       /* the pitch; its tone, a low-pass or a high-pass) */
    float sstep;
    uint8_t shp;
} pc_t;
static const uint8_t *volatile p_sd[PERC_VOICES];   /* (LOAF BREAKER 0.9.45: each drum's sample, perc_set_sample) */
static volatile uint32_t p_sl[PERC_VOICES], p_sr[PERC_VOICES];
BRK_COLD void perc_set_sample(uint32_t voice, const uint8_t *adpcm, uint32_t len, uint32_t rate)
{
    voice %= PERC_VOICES;
    p_sd[voice] = 0;                     /* (the render sees none, or all of the new one: the pointer last) */
    p_sl[voice] = len;
    p_sr[voice] = rate;
    p_sd[voice] = len && rate ? adpcm : 0;
}
BRK_COLD float perc_sample_ms(float d)
{
    d = brk_clampf(d, 0.0f, 1.0f);
    return d >= 0.99f ? 0.0f : 30.0f + 1500.0f * d * d;
}
static volatile uint8_t p_x[PERC_VOICES][PX_N];   /* (LOAF BREAKER: the extras, perc_set_x) */
void perc_set_x(uint32_t voice, const uint8_t *x)
{
    uint32_t j;
    for (j = 0; j < PX_N; j++)
        p_x[voice % PERC_VOICES][j] = x[j];
}

static volatile perc_t p_p;
static volatile uint32_t p_hits[PERC_VOICES];
static uint32_t hits_seen[PERC_VOICES];
static pv_t pv[PERC_VOICES];
static uint32_t rng = 0x2545F491u;

static inline float noise(void)
{
    rng = rng * 1664525u + 1013904223u;
    return (float)(int32_t)rng * (1.0f / 2147483648.0f);
}

BRK_COLD static void pc_make(uint32_t voice, const perc_voice_t *s, float volume, pc_t *c)
{
    float snap = brk_clampf(s->snap, 0.0f, 1.0f), tone = brk_clampf(s->tone, 0.0f, 1.0f);
    float dr = brk_clampf(s->drive, 0.0f, 1.0f);
    float ms = perc_decay_ms(voice, s->decay);
    c->f = perc_tune_hz(voice, s->tune) / (float)BRK_SR;
    c->lvl = brk_clampf(s->level, 0.0f, 1.0f) * brk_clampf(volume, 0.0f, 1.0f);
    c->g = 1.0f + 15.0f * dr * dr;       /* the drive, made back up to a full-scale peak */
    c->make = (1.0f + c->g) / c->g;
    {                                    /* LOAF BREAKER: the extras (all 0: as it always was) */
        const volatile uint8_t *x = p_x[voice % PERC_VOICES];
        float t = (float)x[PX_DTIME];
        c->depth = x[PX_DROP] ? 0.7f + 0.043f * (float)x[PX_DROP] : 0.7f;
        c->kpe = x[PX_DTIME] ? brk_exp2f_c(-1.4427f / (3.0f * brk_exp2f_c(t * (1.0f / 18.0f)) * 0.001f * (float)BRK_SR))
                             : 0.9985f;
        c->dirt = x[PX_DIRT] % DIRT_N;
        c->lift = (float)x[PX_LIFT] * 0.01f;
        c->room = (float)x[PX_ROOM] * 0.01f;
        c->gate = (uint32_t)((40.0f + 5.0f * (float)x[PX_ROOM]) * 0.001f * (float)BRK_SR);
    }
    c->sdat = 0;
    if (p_x[voice % PERC_VOICES][PX_RSV] == PX_SAMPLE && p_sd[voice % PERC_VOICES]) {   /* (0.9.45: its sample) */
        float sms = perc_sample_ms(s->decay);
        c->sdat = p_sd[voice % PERC_VOICES];
        c->slen = p_sl[voice % PERC_VOICES];
        c->sstep = (float)p_sr[voice % PERC_VOICES] * (1.0f / (float)BRK_SR) *
                   brk_exp2f_c((brk_clampf(s->tune, 0.0f, 1.0f) - 0.5f) * 2.0f);
        c->dec = sms > 0.0f ? decay_factor(sms) : 1.0f;
        c->shp = tone > 0.52f;           /* TONE: left a low-pass 18 kHz .. 400 Hz, right a high-pass 20 Hz .. 3 kHz */
        c->hpk = tone > 0.52f ? onepole(20.0f * brk_exp2f_c((tone - 0.52f) * 15.0f)) : 0.0f;
        c->lpk = tone < 0.48f ? onepole(400.0f * brk_exp2f_c(tone * 11.4f)) : 1.0f;
        return;
    }
    if (voice == PV_SNARE) {
        c->dec = decay_factor(ms * 0.55f);           /* the body: shorter than the noise */
        c->dec2 = decay_factor(ms);
        c->hpk = onepole(700.0f * brk_exp2f_c(tone * 2.2f));
        c->lpk = onepole(3500.0f * brk_exp2f_c(tone * 1.5f));
        c->body = 1.0f - 0.6f * snap;
        c->noise = 0.35f + 0.85f * snap;
        c->crack = snap;
    } else if (voice == PV_CLAP) {       /* LOAF BREAKER: the bursts' band, their spacing, the tail */
        float hz = c->f * (float)BRK_SR;
        c->dec = decay_factor(ms);
        c->dec2 = decay_factor(14.0f);   /* a burst: gone in 14 ms */
        c->hpk = onepole(hz * 0.6f);
        c->lpk = onepole(hz * (1.2f + 1.5f * tone));
        c->body = 0.0f;
        c->crack = (4.0f + 14.0f * snap) * 0.001f * (float)BRK_SR;   /* samples between bursts */
        c->noise = 2.5f;
    } else {
        c->dec = c->dec2 = decay_factor(ms);
        c->hpk = onepole(3000.0f * brk_exp2f_c(snap * 1.74f));
        c->lpk = 0.0f;
        c->body = 1.0f - tone;           /* metal against noise */
        c->noise = tone;
        c->crack = 0.0f;
    }
}

static void pv_hit(pv_t *v)
{
    uint32_t k;
    v->declick = v->last;                /* what still sounds fades out under the new hit */
    v->env = v->env2 = v->pe = 1.0f;
    for (k = 0; k < 6u; k++)
        v->ph[k] = 0.0f;
    v->sst.pred = 0;                     /* (0.9.45: its sample from the start) */
    v->sst.index = 0u;
    v->spos = 0u;
    v->sa = v->sb = 0.0f;
    v->sf = 1.0f;
    v->sact = 1u;
}

/* LOAF BREAKER (0.9.45): the voice's sample, decoded a sample at a time as the pitch asks, read between two; the fade,
 * the tone */
static inline float sample_tick(pv_t *v, const pc_t *c)
{
    float x;
    if (!v->sact)
        return 0.0f;
    v->sf += c->sstep;
    while (v->sf >= 1.0f) {
        v->sf -= 1.0f;
        v->sa = v->sb;
        if (v->spos < c->slen)
            v->sb = (float)adpcm_at(&v->sst, c->sdat, v->spos++) * (1.0f / 32768.0f);
        else if (v->sb == 0.0f && v->sa == 0.0f) {
            v->sact = 0u;                /* (its end: quiet) */
            return 0.0f;
        } else
            v->sb = 0.0f;
    }
    x = (v->sa + (v->sb - v->sa) * v->sf) * v->env;
    v->env *= c->dec;
    if (v->env < 1e-5f)
        v->sact = 0u;
    v->lp += (x - v->lp) * c->lpk;       /* (TONE: the low-pass, open at the middle) */
    x = v->lp;
    if (c->shp) {
        v->hp1 += (x - v->hp1) * c->hpk;
        x -= v->hp1;
    }
    return x;
}

static inline float snare_tick(pv_t *v, const pc_t *c)
{
    float b, n = noise(), x;
    b = 0.6f * brk_sin_turns(v->ph[0]) + 0.4f * brk_sin_turns(v->ph[1]);
    v->ph[0] += c->f * (1.0f + c->depth * v->pe);
    v->ph[1] += c->f * 1.62f * (1.0f + c->depth * v->pe);
    if (v->ph[0] >= 1.0f)
        v->ph[0] -= 1.0f;
    if (v->ph[1] >= 1.0f)
        v->ph[1] -= 1.0f;
    v->pe *= c->kpe;                     /* (the body's drop: ~15 ms; LOAF BREAKER: PX_DTIME's) */
    v->hp1 += (n - v->hp1) * c->hpk;     /* noise: a high-pass then a low-pass, a band */
    v->lp += ((n - v->hp1) - v->lp) * c->lpk;
    x = b * v->env * c->body + v->lp * v->env2 * (c->noise + c->crack * 2.0f * v->pe * v->pe);
    v->env *= c->dec;
    v->env2 *= c->dec2;
    return x;
}

static inline float hat_tick(pv_t *v, const pc_t *c)
{
    float m = 0.0f, x, h;
    uint32_t k;
    for (k = 0; k < 6u; k++) {           /* six squares */
        v->ph[k] += c->f * HAT_RATIO[k] * 2.0f;
        if (v->ph[k] >= 1.0f)
            v->ph[k] -= 1.0f;
        m += v->ph[k] < 0.5f ? 1.0f : -1.0f;
    }
    x = m * (1.0f / 6.0f) * c->body + noise() * c->noise;
    v->hp1 += (x - v->hp1) * c->hpk;     /* two high-passes at CUT */
    h = x - v->hp1;
    v->hp2 += (h - v->hp2) * c->hpk;
    x = (h - v->hp2) * 2.5f * v->env;
    v->env *= c->dec;
    return x;
}

/* LOAF BREAKER: the clap: three more bursts c->crack samples apart (ph[0] counting, ph[1] how many), then the tail */
static inline float clap_tick(pv_t *v, const pc_t *c)
{
    float n = noise(), x;
    v->hp1 += (n - v->hp1) * c->hpk;
    v->lp += ((n - v->hp1) - v->lp) * c->lpk;
    if (v->ph[1] < 3.0f) {
        v->ph[0] += 1.0f;
        if (v->ph[0] >= c->crack) {
            v->ph[0] = 0.0f;
            v->ph[1] += 1.0f;
            v->pe = 0.7f;                /* (the first burst the loudest) */
        }
        x = v->lp * v->pe;
    } else {                             /* the tail under the last burst */
        float t = 0.5f * v->env;
        x = v->lp * (v->pe > t ? v->pe : t);
        v->env *= c->dec;
    }
    v->pe *= c->dec2;
    return x * c->noise;
}

static inline float voice_tick(uint32_t k, pv_t *v, const pc_t *c)
{
    if (c->sdat)
        return sample_tick(v, c);
    return k == PV_SNARE ? snare_tick(v, c) : k == PV_CLAP ? clap_tick(v, c) : hat_tick(v, c);
}
/* LOAF BREAKER: a voice's sample out: its saturation (SOFT as ever, HARD, FOLD, BITS: 5 bits held 3 samples), LIFT
 * (quiet up, loud down: the gain worked out every 16th sample), its level */
static inline float voice_out(pv_t *v, const pc_t *c, float x)
{
    float u = x * c->g, y;
    if (c->dirt == DIRT_SOFT)
        y = soft(u) * c->make;
    else if (c->dirt == DIRT_HARD)
        y = brk_clampf(u, -1.0f, 1.0f);
    else if (c->dirt == DIRT_FOLD) {
        float t = 0.25f * u + 0.25f;
        t -= (float)(int32_t)(t + 1000.0f) - 1000.0f;
        y = 1.0f - 4.0f * brk_absf(t - 0.5f);
    } else {
        if (!(v->ln % 3u)) {
            float q = brk_clampf(u, -1.0f, 1.0f) * 16.0f;
            v->bits = (float)(int32_t)(q + (q < 0.0f ? -0.5f : 0.5f)) * (1.0f / 16.0f);
        }
        y = v->bits;
    }
    if (c->lift > 0.0f) {
        float a = brk_absf(y);
        v->lenv += (a - v->lenv) * (a > v->lenv ? 0.05f : 0.0008f);
        if (!(v->ln & 15u))
            v->lg = brk_clampf(brk_exp2f(c->lift * (-2.0f - brk_log2f(v->lenv + 1e-3f))), 0.3f, 6.0f);
        y *= v->lg;
    }
    v->ln++;
    return y * c->lvl;
}
/* LOAF BREAKER: the drums' short room (Schroeder: four combs, two allpasses; 16-bit lines), gated: open for the longest
 * hit's PX_ROOM length, then closed in ~6 ms */
#define RM_C 4u
static const uint16_t RM_CL[RM_C] = {557u, 617u, 691u, 751u}, RM_AL[2] = {225u, 76u};
static int16_t rm_c[RM_C][751] BRK_POOL, rm_a[2][225] BRK_POOL;   /* (0.9.71: in the pool, 6.9 KB of RAM freed) */
static uint16_t rm_cp[RM_C], rm_ap[2];
static float rm_f[RM_C], rm_g;
static uint32_t rm_open;
static inline float room_tick(float in)
{
    float s = 0.0f;
    uint32_t k;
    for (k = 0; k < RM_C; k++) {
        float o = (float)rm_c[k][rm_cp[k]] * (1.0f / 8192.0f);
        rm_f[k] += (o - rm_f[k]) * 0.6f;
        rm_c[k][rm_cp[k]] = (int16_t)brk_clampf((in + rm_f[k] * 0.82f) * 8192.0f, -32767.0f, 32767.0f);
        if (++rm_cp[k] >= RM_CL[k])
            rm_cp[k] = 0u;
        s += o;
    }
    for (k = 0; k < 2u; k++) {
        float o = (float)rm_a[k][rm_ap[k]] * (1.0f / 8192.0f), w = s + o * 0.5f;
        rm_a[k][rm_ap[k]] = (int16_t)brk_clampf(w * 8192.0f, -32767.0f, 32767.0f);
        if (++rm_ap[k] >= RM_AL[k])
            rm_ap[k] = 0u;
        s = o - w * 0.5f;
    }
    rm_g = rm_open ? 1.0f : rm_g * 0.9962f;
    if (rm_open)
        rm_open--;
    return s * rm_g * 2.2f;
}

BRK_COLD void perc_init(void)
{
    uint32_t k;
    for (k = 0; k < PERC_VOICES; k++) {
        pv[k].env = pv[k].env2 = pv[k].pe = 0.0f;
        pv[k].last = pv[k].declick = 0.0f;
        pv[k].ph[1] = 3.0f;              /* (LOAF BREAKER: the clap's bursts done: none until a hit) */
        pv[k].lg = 1.0f;                 /* (LIFT: untouched until it has followed something) */
        hits_seen[k] = p_hits[k];
    }
}
BRK_COLD void perc_set(const perc_t *p) { *(perc_t *)&p_p = *p; }
void perc_hit(uint32_t voice)
{
    if (voice < PV_STAB)                 /* (the drums: the MELODY's place empty) */
        p_hits[voice]++;
}

static const float *vs_sp;               /* LOAF BREAKER: the vinyl's speed (perc_speed), as the kick's */
static float vs_ph, vs_a, vs_b, vs_last;
static uint8_t vs_on;
void perc_speed(const float *sp) { vs_sp = sp; }

void perc_render(float *out, uint32_t n, const uint16_t *at, const uint8_t *who, uint32_t nat)
{
    perc_t s = *(const perc_t *)&p_p;
    pc_t c[PERC_VOICES];
    uint32_t i, j, k, hit = nat ? 1u : 0u;
    uint32_t mh = 0u;
    for (k = 0; k < PV_STAB; k++)
        if (p_hits[k] != hits_seen[k]) {     /* a MIDI note: now */
            hits_seen[k] = p_hits[k];
            pv_hit(&pv[k]);
            hit = 1u;
            mh |= 1u << k;
        }
    if (!hit && !rm_open && rm_g < 1e-4f && !(pv[0].sact | pv[1].sact | pv[2].sact) && pv[0].env < 3e-5f && pv[0].env2 < 3e-5f && pv[1].env < 3e-5f && pv[2].env < 3e-5f &&
        pv[2].pe < 3e-5f && brk_absf(pv[0].declick) + brk_absf(pv[1].declick) + brk_absf(pv[2].declick) < 1e-5f) {
        for (i = 0; i < n; i++)          /* quiet, nothing coming: no work */
            out[i] = 0.0f;
        pv[0].last = pv[1].last = pv[2].last = vs_last = 0.0f;
        vs_on = 0u;
        return;
    }
    for (k = 0; k < PV_STAB; k++) {      /* (the drums: snare, hat, clap) */
        pc_make(k, &s.v[k], s.volume, &c[k]);
        if (mh >> k & 1u && c[k].room > 0.0f && c[k].gate > rm_open)
            rm_open = c[k].gate;         /* (LOAF BREAKER: a MIDI hit opens the room) */
    }
    if (vs_sp && !vs_on) {               /* the vinyl begins: from the drums as they are */
        vs_a = vs_b = vs_last;
        vs_ph = 0.0f;
    }
    vs_on = vs_sp != 0;
    for (i = 0, j = 0; i < n; i++) {
        float y = 0.0f, f = 1.0f;
        while (j < nat && at[j] <= i) {
            for (k = 0; k < PV_STAB; k++)
                if ((who[j] >> k) & 1u) {
                    pv_hit(&pv[k]);
                    if (c[k].room > 0.0f && c[k].gate > rm_open)
                        rm_open = c[k].gate;   /* (LOAF BREAKER: its room open for its length) */
                }
            j++;
        }
        if (vs_sp) {                     /* the vinyl: their own time at the speed, read between two samples */
            f = vs_sp[i] < 0.02f ? vs_sp[i] * 50.0f : 1.0f;
            vs_ph += vs_sp[i];
            if (vs_ph < 1.0f) {
                out[i] = (vs_a + (vs_b - vs_a) * vs_ph) * f;
                continue;
            }
            vs_ph -= 1.0f;
        }
        {
            float rs = 0.0f;             /* (LOAF BREAKER: what goes to the room) */
            for (k = 0; k < PV_STAB; k++) {
                pv_t *v = &pv[k];
                float x = 0.0f;
                if (c[k].sdat ? v->sact : v->env > 1e-5f || (k == PV_SNARE && (v->env2 > 1e-5f)) ||
                    (k == PV_CLAP && (v->pe > 1e-5f || v->ph[1] < 3.0f)))   /* (LOAF BREAKER, 0.226: a voice gone
                                          * quiet costs nothing) */
                    x = voice_out(v, &c[k], voice_tick(k, v, &c[k]));
                v->last = x;
                y += x + v->declick;
                v->declick *= 0.985f;
                rs += x * c[k].room;
            }
            if (rs != 0.0f || rm_open || rm_g > 1e-4f)
                y += room_tick(rs * 0.25f);
        }
        if (vs_sp) {
            vs_a = vs_b;
            vs_b = y;
            y = (vs_a + (vs_b - vs_a) * vs_ph) * f;
        }
        out[i] = y;
    }
    vs_last = out[n - 1u];
}

BRK_COLD void perc_shape(uint32_t voice, const perc_voice_t *s, uint8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    pc_t c;
    pv_t v;
    uint32_t i, k, per = ms * (BRK_SR / 1000u) / (n ? n : 1u);
    voice %= PERC_VOICES;
    pc_make(voice, s, 1.0f, &c);
    for (k = 0; k < 6u; k++)
        v.ph[k] = 0.0f;
    v.hp1 = v.hp2 = v.lp = v.last = v.declick = 0.0f;
    pv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t j;
        float pk = 0.0f;
        for (j = 0; j < per; j++) {
            float x = voice_tick(voice, &v, &c);
            x = brk_absf(soft(x * c.g) * c.make * c.lvl);
            if (x > pk)
                pk = x;
        }
        px[i] = (uint8_t)(brk_clampf(pk, 0.0f, 1.0f) * (float)height + 0.5f);
    }
}

BRK_COLD void perc_wave(uint32_t voice, const perc_voice_t *s, int8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    pc_t c;
    pv_t v;
    uint32_t i, k, per = ms * (BRK_SR / 1000u) / (n ? n : 1u);
    voice %= PERC_VOICES;
    pc_make(voice, s, 1.0f, &c);
    for (k = 0; k < 6u; k++)
        v.ph[k] = 0.0f;
    v.hp1 = v.hp2 = v.lp = v.last = v.declick = 0.0f;
    pv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t j;
        float best = 0.0f;
        for (j = 0; j < per; j++) {
            float x = soft(voice_tick(voice, &v, &c) * c.g) * c.make * c.lvl;
            if (brk_absf(x) > brk_absf(best))
                best = x;
        }
        px[i] = (int8_t)(brk_clampf(best, -1.0f, 1.0f) * (float)height);
    }
}

/* LOAF BREAKER (0.231, FLEXOR: the KICK's RGB view for the drums too): one hit's lows, mids, highs and peak a column,
 * each band followed (~15 ms down), as kick_rgb: the KICK menu's colours, rekordbox's or SERATO's */
BRK_COLD void perc_rgb(uint32_t voice, const perc_voice_t *s, uint8_t (*col)[4], uint32_t n, uint32_t ms)
{
    static const float G[4] = {300.0f, 380.0f, 520.0f, 255.0f};
    pc_t c;
    pv_t v;
    uint32_t i, j, k, per = ms * (BRK_SR / 1000u) / (n ? n : 1u);
    float l1 = 0.0f, l2 = 0.0f, e[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    voice %= PERC_VOICES;
    pc_make(voice, s, 1.0f, &c);
    for (k = 0; k < 6u; k++)
        v.ph[k] = 0.0f;
    v.hp1 = v.hp2 = v.lp = v.last = v.declick = 0.0f;
    pv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t t;
        float pk[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (t = 0; t < per; t++) {
            float x = soft(voice_tick(voice, &v, &c) * c.g) * c.make * c.lvl, b[4];
            l1 += (x - l1) * 0.0285f;    /* (~200 Hz) */
            l2 += (x - l2) * 0.356f;     /* (~2.5 kHz) */
            b[0] = brk_absf(l1);
            b[1] = brk_absf(l2 - l1);
            b[2] = brk_absf(x - l2);
            b[3] = brk_absf(x);
            for (j = 0; j < 4u; j++) {
                e[j] = b[j] > e[j] ? b[j] : e[j] * 0.9985f;
                pk[j] = e[j] > pk[j] ? e[j] : pk[j];
            }
        }
        for (j = 0; j < 4u; j++)
            col[i][j] = (uint8_t)brk_clampf(pk[j] * G[j], 0.0f, 255.0f);
    }
}

/* the built-in sounds: level, tune, decay, tone, snap (the hat's cut), drive */
static const float PRESET[PERC_VOICES][PERC_PRESETS][6] = {
    {{0.80f, 0.45f, 0.40f, 0.50f, 0.55f, 0.10f},    /* 909 */
     {0.75f, 0.65f, 0.20f, 0.70f, 0.70f, 0.20f},    /* TIGHT */
     {0.85f, 0.25f, 0.65f, 0.35f, 0.45f, 0.30f},    /* FAT */
     {0.85f, 0.55f, 0.45f, 0.60f, 0.75f, 0.85f}},   /* GABBER */
    {{0.60f, 0.55f, 0.10f, 0.55f, 0.55f, 0.00f},    /* CLOSED */
     {0.60f, 0.35f, 0.18f, 0.20f, 0.45f, 0.10f},    /* 808 */
     {0.55f, 0.50f, 0.75f, 0.50f, 0.50f, 0.05f},    /* OPEN */
     {0.65f, 0.80f, 0.15f, 0.85f, 0.70f, 0.80f}},   /* TRASH */
    {{0.75f, 0.35f, 0.45f, 0.40f, 0.45f, 0.10f},    /* CLAP 808 (snap: the bursts' spread) */
     {0.75f, 0.55f, 0.35f, 0.60f, 0.35f, 0.15f},    /* 909 */
     {0.70f, 0.60f, 0.15f, 0.55f, 0.20f, 0.10f},    /* TIGHT */
     {0.80f, 0.30f, 0.65f, 0.35f, 0.70f, 0.30f}},   /* BIG */
    {{0.0f}},                                      /* (the MELODY's place, empty) */
};
BRK_COLD void perc_preset(perc_voice_t *v, uint32_t voice, uint32_t i)
{
    const float *p = PRESET[voice % PERC_VOICES][i % PERC_PRESETS];
    v->level = p[0];
    v->tune = p[1];
    v->decay = p[2];
    v->tone = p[3];
    v->snap = p[4];
    v->drive = p[5];
}

BRK_COLD void perc_defaults(perc_t *p)
{
    uint32_t k;
    for (k = 0; k < PERC_VOICES; k++)
        perc_preset(&p->v[k], k, 0u);
    p->volume = 0.8f;
}
