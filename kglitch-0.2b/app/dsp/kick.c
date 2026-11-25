/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's kick (kick.h). */
#define BRK_ENGINE 1                     /* (LOAF BREAKER, 0.226: the maths inlined into the per-sample code, as
                                          * brk_fx.c's: kv_tick called brk_sin_turns up to eight times a sample) */
#include "kick.h"
#include "brk.h"
#include "brk_math.h"

const char *const KICK_LAYER_NAME[KICK_LAYERS] = {"SUB", "PUNCH", "GRIT", "CLICK"};
const char *const KICK_FX_NAME[KICK_FX] = {"OFF", "DRIVE", "CRUNCH", "FILTER", "RING MOD"};
const char *const KICK_PRESET_NAME[KICK_PRESETS] = {"909", "TECHNO", "EARLY HC", "ROTTERDAM", "INDUSTRIAL", "DISTORTO",
                                                     "TERROR", "SPINE", "909 DRIVE", "GABBER", "MILLENNIUM",
                                                     "DOOMCORE"};
/* (0.9.39, FLEXOR: PRESET's round, K1 / OUT K4; DISTORTO out of it, its place kept for the kicks saved from it) */
const uint8_t KICK_PRESET_ORD[KICK_PRESETS_ON] = {0u, 8u, 1u, 2u, 3u, 4u, 6u, 7u, 9u, 10u, 11u};

static inline float soft(float x) { return x / (1.0f + brk_absf(x)); }

/* one kick sounding, its layers' state (the screen's shape renders its own) */
typedef struct {
    float ph[KICK_LAYERS], ph2, env[KICK_LAYERS], pe, pe2;
    float lp;                            /* CLICK: its noise's low-pass (what it keeps is above it) */
    float tlo, tlp, tenv;                /* LOAF BREAKER: TERROR's low split, its fizz's low-pass, the kick's level */
    float t2lo, t2lp, t2env;             /* TERROR 2's low split, its mids' low-pass, the kick's level */
    float sc1, sc2;                      /* (TEST) TERROR's SCREECH, its band's state */
    float rdc;                           /* LOAF BREAKER: TERROR's RECT and TUBE, their offset followed (taken off) */
    float clp;                           /* CLEAN SUB: the distorted kick's lows (taken out) */
    uint32_t hold;                       /* (SYNTH) the SUB's HOLD left, samples (~0u: from the hit, to be set) */
    float rlp, rout, rlvl, dc;           /* the RUMBLE: its input low-passed, its output low-passed, its level; ASYM's
                                          * offset followed (taken off) */
    float pm;                            /* (crossbreed) this hit's pitch, a factor (kick_pitches): 1 as set */
    float pm_t;                          /* (TEST) GLIDE: the pitch it slides to */
    uint32_t rb_t;                       /* (TEST) REVERSE BASS: samples since the hit */
    float rb_ph;                         /* its phase */
    float s1[KICK_LAYERS], s2[KICK_LAYERS], fph[KICK_LAYERS], fhold[KICK_LAYERS];   /* the layers' effects */
    uint32_t rng;
    float last, declick;
    uint32_t t;                          /* LOAF BREAKER LAB: samples since the hit */
    float pl, gend, fenv, f1, f2;        /* the drop's ramp (LINEAR, SNAP), END's gain, the TAIL FILTER's fall, its
                                          * two poles */
    float tok_ph, tok_env, tok_pe, tok_n1, tok_n2;   /* (0.9.45) TOK: its phase, level, pitch's fall, its filters */
    uint32_t g_pos;                      /* the TAIL GATE: where in its chop */
    float g_s;                           /* its gain, smoothed (no clicks) */
    float sp_env, sp_g;                  /* (0.9.71) TAIL MODE SPLIT: the kick's level followed, the clean sine's level
                                          * against the SUB's (0: not split yet) */
} kv_t;

/* the block's settings, mapped */
typedef struct {
    float f[KICK_LAYERS];                /* each layer's frequency step (cycles a sample) */
    float dec[KICK_LAYERS];              /* each envelope's factor a sample */
    float lvl[KICK_LAYERS];
    float sub_sat, punch_sweep, grit_index, grit_noise, click_noise;
    float click_k;                       /* the click's high-pass coefficient */
    float pe_dec, pe2_dec;               /* the pitch sweeps' factors */
    /* the layers' effects */
    uint8_t fx[KICK_LAYERS];
    float fa[KICK_LAYERS], fb[KICK_LAYERS], wet[KICK_LAYERS];
    float ga[KICK_LAYERS], gb[KICK_LAYERS], gc[KICK_LAYERS];   /* the effect's own coefficients */
    float g, hard, make, vol;            /* the sum: drive, how hard the clip, make-up, volume */
    float tmix, tg;                      /* LOAF BREAKER: TERROR, its mix and its drive (0 mix: off) */
    float t2mix, t2g;                    /* TERROR 2, its mix and its drive */
    float tsub, tfz, tsus;               /* (ENV's KNOB) TERROR's split, its fizz's low-pass, its follow */
    float t2sub, t2scr, t2push;          /* TERROR 2's split, its mids' low-pass, their push */
    float trel;                          /* (TEST) TERROR's TAIL: its level's fall a sample */
    uint8_t ttype;                       /* its TYPE (0 FOLD, 1 CLIP, 2 CRUSH, 3 RECT, 4 TUBE, 5 HALF, 6 SINE, 7 WRAP,
                                          * 8 FUZZ: KICK_T_TYPES) */
    float csub;                          /* LOAF BREAKER: CLEAN SUB, 0..1 */
    float sa1, sa2, sa3, sgain;          /* its SCREECH: the band's coefficients and level (0: off) */
    float sdrop, asym, rum, rfb;         /* (SYNTH) the SUB's drop, the uneven drive, the rumble's level and feedback */
    uint32_t hold_n, rlen;               /* the SUB's hold (samples), the rumble's line (samples) */
    float rb_lvl, rb_k;                  /* (TEST) REVERSE BASS: its level, its curve's power (1..4) */
    uint32_t rb_s, rb_e;                 /* where it starts and ends, samples from the hit */
    uint8_t clean, curve, filt;          /* LOAF BREAKER LAB: CLEAN, the drop's shape, the TAIL FILTER on */
    float grat, pl_step, fa_end, f_dec, tdrive;   /* GRIT's FM ratio, the ramp's step, the filter's coefficient at its
                                          * end, its fall a sample, TAIL DRIVE */
    uint32_t end_n, x0, x1;              /* END (0 off), STACK's hand-over from .. to (samples from the hit) */
    uint32_t sp_n, sp_f;                 /* (0.9.71) TAIL MODE SPLIT: where the tail turns clean, the fade's samples (0:
                                          * no SPLIT) */
    float sp_k, dr_k;                    /* its fade's step a sample; DIRTY: its rise's step (0: no DIRTY) */
    float tok_lvl, tok_f1, tok_fd, tok_pdec, tok_edec, tok_k;   /* (0.9.45) TOK: level (0 off), its end frequency and
                                          * the drop above it (cycles a sample), their falls, NOISE's band coefficient */
    uint8_t tok_type;
    uint32_t g_len, g_on, g_start, g_fade_n;   /* the TAIL GATE: a chop's samples (0 off), those that sound, where it
                                          * starts, its fade in (0 at once) */
    float g_fade_k;
} kc_t;
static volatile kick_x2_t p_x2;          /* (0.9.45) kick_set_x2 */
BRK_COLD void kick_set_x2(const kick_x2_t *x) { *(kick_x2_t *)&p_x2 = *x; }
static volatile float p_glide, p_rbl, p_rbs, p_rbk, p_spb = 15562.0f;   /* (TEST) kick_set_bass, kick_set_beat */
void kick_set_bass(float glide_ms, float rb_level, float rb_start, float rb_curve)
{
    p_glide = glide_ms;
    p_rbl = rb_level;
    p_rbs = rb_start;
    p_rbk = rb_curve;
}
void kick_set_beat(float spb) { p_spb = spb; }
static volatile float p_csub;            /* LOAF BREAKER: CLEAN SUB (kick_set_clean_sub) */
void kick_set_clean_sub(float amount) { p_csub = amount; }

static volatile kick_t p_k;
static volatile int8_t p_syn[KS_N];      /* (SYNTH) the main loop's */
static volatile kick_lab_t p_lab;        /* LOAF BREAKER LAB: the kick playing's */
static volatile kick_t p_ak;             /* STACK's attack: its kick, SYNTH, LAB; on */
static volatile int8_t p_asyn[KS_N];
static volatile kick_lab_t p_alab;
static volatile uint8_t p_aon, p_solo;
static kv_t kv2;                         /* the attack's voice */
BRK_COLD void kick_set_lab(const kick_lab_t *l) { *(kick_lab_t *)&p_lab = *l; }
BRK_COLD void kick_set_stack(const kick_t *k, const int8_t *syn, const kick_lab_t *l)
{
    uint32_t i;
    if (!k) {
        p_aon = 0u;
        return;
    }
    p_aon = 0u;                          /* (not read half-written) */
    *(kick_t *)&p_ak = *k;
    for (i = 0; i < KS_N; i++)
        p_asyn[i] = syn[i];
    *(kick_lab_t *)&p_alab = *l;
    p_aon = 1u;
}
void kick_set_solo(uint32_t s) { p_solo = (uint8_t)(s > 2u ? 0u : s); }
BRK_COLD float kick_lab_ms(uint32_t what, uint32_t v)
{
    float f = (float)(v > 100u ? 100u : v);
    switch (what) {
    case KLM_END:                        /* (1..49: 40 .. 1000 ms by 20 ms, as saved before; 0.9.70, FLEXOR: 50..89 on to
                                          * 3 s by 50 ms) */
        return f <= 49.0f ? 20.0f + 20.0f * f : 1000.0f + 50.0f * (f - 49.0f);
    case KLM_CROSS:
        return 10.0f * brk_exp2f_c(f * 0.049f);
    case KLM_FADE:
        return 2.0f * brk_exp2f_c(f * 0.0623f);
    case KLM_FTIME:
        return 20.0f * brk_exp2f_c(f * 0.049f);
    case KLM_FILT:
        return 12000.0f * brk_exp2f_c(-f * 0.06f);
    case KLM_SPLIT:                      /* (0.9.71: 30 .. 600 ms) */
        return 30.0f * brk_exp2f_c(f * 0.0432f);
    case KLM_RISE:                       /* (0.9.71: 50 .. 2000 ms) */
        return 50.0f * brk_exp2f_c(f * 0.0532f);
    default:                             /* (KLM_NOTE: MIDI 22 + n, A#0 ..) */
        return 440.0f * brk_exp2f_c((f + 22.0f - 69.0f) * (1.0f / 12.0f));
    }
}
static int16_t *rum_buf;                 /* the rumble's line (the app's), its length, where it is */
static uint32_t rum_cap, rum_pos;
void kick_set_synth(const int8_t *s)
{
    uint32_t i;
    for (i = 0; i < KS_N; i++)
        p_syn[i] = s[i];
}
void kick_rumble_buf(int16_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
        buf[i] = 0;
    rum_buf = buf;
    rum_cap = len;
    rum_pos = 0u;
}
static volatile uint32_t p_hits;         /* a count: kick_hit */
static uint32_t hits_seen;
static kv_t kv;
static const float *ks_sp;               /* LOAF BREAKER: the vinyl's speed (kick_speed), 0 none */
static float ks_ph, ks_a, ks_b, ks_last; /* its phase between two of the kick's samples, the two; the last out */
static uint8_t ks_on;

/* the factor a sample that falls 40 dB (to where a kick is heard as gone) in ms milliseconds */
static float decay_factor(float ms) { return brk_exp2f_c(-6.644f / (ms * 0.001f * (float)BRK_SR)); }

/* the knobs' 0..1 as each layer's frequency and its decay (to -40 dB) */
static const float P_LO[KICK_LAYERS] = {30.0f, 50.0f, 40.0f, 1000.0f};
static const float P_OCT[KICK_LAYERS] = {1.585f, 2.0f, 3.32f, 3.585f};
static const float D_LO[KICK_LAYERS] = {60.0f, 20.0f, 20.0f, 1.0f};
static const float D_HI[KICK_LAYERS] = {1500.0f, 400.0f, 800.0f, 40.0f};
BRK_COLD float kick_pitch_hz(uint32_t layer, float p)
{
    layer %= KICK_LAYERS;
    return P_LO[layer] * brk_exp2f_c(brk_clampf(p, 0.0f, 1.0f) * P_OCT[layer]);
}
BRK_COLD float kick_decay_ms(uint32_t layer, float d)
{
    layer %= KICK_LAYERS;
    d = brk_clampf(d, 0.0f, 1.0f);
    return D_LO[layer] + (D_HI[layer] - D_LO[layer]) * d * d;
}

BRK_COLD float kick_fx_hz(uint32_t fx, float a)
{
    a = brk_clampf(a, 0.0f, 1.0f);
    return fx == KFX_FILTER ? 60.0f * brk_exp2f_c(a * 7.6f) : 30.0f * brk_exp2f_c(a * 6.0f);   /* 60..11.6k; 30..1.9k */
}
uint32_t kick_fx_bits(float a) { return 15u - (uint32_t)(brk_clampf(a, 0.0f, 1.0f) * 12.0f + 0.5f); }   /* 15..3 */

BRK_COLD static void kc_make(const kick_t *k, kc_t *c, const volatile int8_t *syn, const kick_lab_t *lab)
{
    uint32_t i;
    for (i = 0; i < KICK_LAYERS; i++) {
        const kick_layer_t *l = &k->l[i];
        float fa = brk_clampf(l->fa, 0.0f, 1.0f), fb = brk_clampf(l->fb, 0.0f, 1.0f);
        c->f[i] = kick_pitch_hz(i, l->pitch) / (float)BRK_SR;
        c->dec[i] = decay_factor(kick_decay_ms(i, l->decay));
        c->lvl[i] = brk_clampf(l->level, 0.0f, 1.0f);
        c->fx[i] = l->fx < KICK_FX ? l->fx : KFX_OFF;
        c->fa[i] = fa;
        c->fb[i] = fb;
        c->wet[i] = brk_clampf(l->wet, 0.0f, 1.0f);
        switch (c->fx[i]) {
        case KFX_DRIVE:                  /* ga: gain, gb: its tone's coefficient */
            c->ga[i] = 1.0f + 24.0f * fa * fa;
            c->gb[i] = 0.05f + 0.95f * fb * fb;
            break;
        case KFX_CRUNCH:                 /* ga: levels, gb: the rate's step */
            c->ga[i] = brk_exp2f_c((float)kick_fx_bits(fa) - 1.0f);
            c->gb[i] = brk_exp2f_c(-fb * 4.5f);
            break;
        case KFX_FILTER: {               /* a 2-pole low-pass: ga g, gb k (resonance), gc a1 */
            float g = brk_tan_pi_c(kick_fx_hz(KFX_FILTER, fa) / (float)BRK_SR);
            c->gb[i] = 2.0f - 1.9f * fb;
            c->gc[i] = 1.0f / (1.0f + g * (g + c->gb[i]));
            c->ga[i] = g;
            break;
        }
        case KFX_RING:
            c->ga[i] = kick_fx_hz(KFX_RING, fa) / (float)BRK_SR;
            break;
        default:
            break;
        }
    }
    c->sub_sat = 1.0f + 4.0f * brk_clampf(k->l[KL_SUB].tone, 0.0f, 1.0f);
    c->punch_sweep = 3.0f + 14.0f * brk_clampf(k->l[KL_PUNCH].tone, 0.0f, 1.0f);
    c->grit_index = 0.5f + 7.0f * brk_clampf(k->l[KL_GRIT].tone, 0.0f, 1.0f);
    c->grit_noise = 0.5f * brk_clampf(k->l[KL_GRIT].tone, 0.0f, 1.0f);
    c->click_noise = brk_clampf(k->l[KL_CLICK].tone, 0.0f, 1.0f);
    c->click_k = brk_clampf(brk_tan_pi_c(c->f[KL_CLICK]) * 2.0f, 0.02f, 0.98f);
    {                                    /* (SYNTH: 0, as it always was) */
        float dr = (float)syn[KS_DROP], tm = (float)syn[KS_TIME], sh = (float)syn[KS_SHAPE];
        float hd = (float)syn[KS_HOLD], rm = (float)syn[KS_RUMBLE], rt = (float)syn[KS_RTIME];
        float tune = brk_exp2f_c(brk_clampf((float)syn[KS_TUNE], -12.0f, 12.0f) * (1.0f / 12.0f));
        uint32_t j;
        c->sdrop = 2.0f * brk_exp2f_c(brk_clampf(dr, -50.0f, 50.0f) * 0.04f);
        c->pe_dec = decay_factor(120.0f * brk_exp2f_c(brk_clampf(tm, -50.0f, 50.0f) * 0.04f));
        c->sub_sat *= brk_exp2f_c(brk_clampf(sh, -50.0f, 50.0f) * 0.06f);
        c->hold_n = hd > 0.0f ? (uint32_t)(brk_clampf(hd, 0.0f, 50.0f) * 5.0f * (float)(BRK_SR / 1000u)) : 0u;
        if (hd < 0.0f)                   /* (below 0: the SUB decays faster) */
            c->dec[KL_SUB] = decay_factor(kick_decay_ms(KL_SUB, k->l[KL_SUB].decay) *
                                          brk_exp2f_c(brk_clampf(hd, -50.0f, 0.0f) * 0.02f));
        c->rum = rm > 0.0f ? brk_clampf(rm, 0.0f, 50.0f) * 0.02f : 0.0f;
        c->rlen = (uint32_t)(2048.0f * brk_exp2f_c(brk_clampf(rt, -50.0f, 50.0f) * 0.02f));
        if (c->rlen > rum_cap)
            c->rlen = rum_cap;
        c->rfb = 0.75f + brk_clampf(rt, -50.0f, 50.0f) * 0.003f;
        c->asym = syn[KS_ASYM] > 0 ? (float)syn[KS_ASYM] * 0.005f : 0.0f;
        for (j = 0; j < KICK_LAYERS; j++)
            c->f[j] *= tune;
    }
    {                                    /* (TEST) the REVERSE BASS */
        float spb = brk_clampf(p_spb, 2000.0f, 60000.0f);
        c->rb_lvl = brk_clampf(p_rbl, 0.0f, 1.0f) * 0.8f;
        c->rb_k = 1.0f + 3.0f * brk_clampf(p_rbk, 0.0f, 1.0f);
        c->rb_s = (uint32_t)(spb * (0.125f + 0.5f * brk_clampf(p_rbs, 0.0f, 1.0f)));
        c->rb_e = (uint32_t)spb;
    }
    if (!c->pe_dec)
        c->pe_dec = decay_factor(120.0f);     /* SUB's drop and GRIT's */
    c->pe2_dec = decay_factor(40.0f);     /* PUNCH's fast sweep */
    {
        float in = brk_clampf(k->intensity, 0.0f, 1.0f), sg;
        c->g = 0.3f + 30.0f * in * in;   /* (0: near clean, the clip only rounding the peaks) */
        c->hard = in > 0.5f ? (in - 0.5f) * 1.6f : 0.0f;
        sg = c->g / (1.0f + c->g);       /* a full-scale sum through the clip: kept there */
        c->make = 0.95f / (sg * (1.0f - c->hard) + c->hard);
    }
    {                                    /* LOAF BREAKER: TERROR, in fully by a third of the way, the drive on up */
        float t = (float)(KICK_TERROR(k) > 100u ? 100u : KICK_TERROR(k)) * 0.01f;
        c->tmix = t * 3.0f > 1.0f ? 1.0f : t * 3.0f;
        c->tg = (2.0f + 60.0f * t * t) * brk_exp2f_c((float)(int8_t)KICK_T_DRIVE(k) * 0.04f);   /* (TEST: DRIVE) */
    }
    {                                    /* (TEST) TERROR's TYPE, TAIL, SCREECH */
        float tl = brk_clampf((float)(int8_t)KICK_T_TAIL(k), -50.0f, 50.0f), sw = (float)KICK_T_SCREECH(k);
        c->ttype = (uint8_t)(KICK_T_TYPE(k) < KICK_T_TYPES ? KICK_T_TYPE(k) : 0u);
        c->trel = 0.0012f * brk_exp2f_c(-tl * 0.07f);   /* (+50: ~11x slower, the fold hangs on; -50: faster) */
        c->sgain = 0.0f;
        if (sw > 0.0f) {                 /* a band at ~500 Hz .. 5 kHz, Q ~6 (Simper's state-variable) */
            float g = brk_tan_pi_c(500.0f * brk_exp2f_c(brk_clampf(sw, 1.0f, 100.0f) * 0.0332f) / (float)BRK_SR), kq = 1.0f / 6.0f;
            c->sa1 = 1.0f / (1.0f + g * (g + kq));
            c->sa2 = g * c->sa1;
            c->sa3 = g * c->sa2;
            c->sgain = 0.5f + 1.5f * sw * 0.01f;
        }
    }
    {                                    /* LOAF BREAKER: TERROR 2, in fully by a quarter of the way, the drive on up */
        float t = (float)(KICK_TERROR2(k) > 100u ? 100u : KICK_TERROR2(k)) * 0.01f;
        c->t2mix = t * 4.0f > 1.0f ? 1.0f : t * 4.0f;
        c->t2g = 3.0f + 80.0f * t * t;
    }
    {                                    /* (their settings: 0 as they always were) */
        float a = (float)(int8_t)KICK_T_SUB(k), b = (float)(int8_t)KICK_T_FIZZ(k), e = (float)(int8_t)KICK_T_SUS(k);
        c->tsub = 0.0155f * brk_exp2f_c(brk_clampf(a, -50.0f, 50.0f) * 0.024f);
        c->tfz = brk_clampf(0.45f * brk_exp2f_c(brk_clampf(b, -50.0f, 50.0f) * 0.03f), 0.03f, 1.0f);
        c->tsus = 2.5f * brk_exp2f_c(brk_clampf(e, -50.0f, 50.0f) * 0.04f);
        a = (float)(int8_t)KICK_T2_SUB(k);
        b = (float)(int8_t)KICK_T2_SCREAM(k);
        e = (float)(int8_t)KICK_T2_PUSH(k);
        c->t2sub = 0.0155f * brk_exp2f_c(brk_clampf(a, -50.0f, 50.0f) * 0.024f);
        c->t2scr = brk_clampf(0.085f * brk_exp2f_c(brk_clampf(b, -50.0f, 50.0f) * 0.04f), 0.01f, 0.6f);
        c->t2push = 1.2f * brk_exp2f_c(brk_clampf(e, -50.0f, 50.0f) * 0.03f);
    }
    c->vol = brk_clampf(k->volume, 0.0f, 1.0f);
    c->csub = brk_clampf(p_csub, 0.0f, 1.0f);
    {                                    /* LOAF BREAKER: the KICK LAB */
        float tune = brk_exp2f_c(brk_clampf((float)syn[KS_TUNE], -12.0f, 12.0f) * (1.0f / 12.0f));
        float tms = 120.0f * brk_exp2f_c(brk_clampf((float)syn[KS_TIME], -50.0f, 50.0f) * 0.04f);
        c->clean = lab->mode == 1u;
        c->curve = (uint8_t)(lab->curve <= 2u ? lab->curve : 0u);
        c->pl_step = 1.0f / (tms * 0.5f * 0.001f * (float)BRK_SR);   /* (LINEAR, SNAP: on the note at half TIME) */
        if (lab->note)
            c->f[KL_SUB] = kick_lab_ms(KLM_NOTE, lab->note > 24u ? 24u : lab->note) * tune / (float)BRK_SR;
        c->grat = 1.41f;
        if (c->clean) {                  /* (CLEAN: one note, PUNCH and GRIT on the SUB's octave, GRIT's FM harmonic) */
            c->f[KL_PUNCH] = c->f[KL_GRIT] = c->f[KL_SUB] * 2.0f;
            c->grat = 1.0f;
            if (c->rlen) {               /* (the RUMBLE's loop: a whole number of the note's periods) */
                float per = 1.0f / c->f[KL_SUB];
                uint32_t m = (uint32_t)((float)c->rlen / per + 0.5f);
                m = m ? m : 1u;
                while (m > 1u && (float)m * per > (float)rum_cap)
                    m--;
                c->rlen = (uint32_t)((float)m * per + 0.5f);
                if (c->rlen > rum_cap)
                    c->rlen = rum_cap;
            }
        }
        c->end_n = lab->end ? (uint32_t)(kick_lab_ms(KLM_END, lab->end > KICK_END_MAX ? KICK_END_MAX : lab->end) * (float)(BRK_SR / 1000u)) : 0u;
        c->filt = lab->filt ? 1u : 0u;
        if (c->filt) {
            c->fa_end = 1.0f - brk_exp2f_c(-6.2832f * 1.4427f * kick_lab_ms(KLM_FILT, lab->filt) / (float)BRK_SR);
            c->f_dec = decay_factor(kick_lab_ms(KLM_FTIME, lab->ftime));
        }
        c->tdrive = (float)(lab->drive > 100u ? 100u : lab->drive) * 0.08f;
        c->sp_n = c->sp_f = 0u;
        c->sp_k = c->dr_k = 0.0f;
        if (lab->mode == KTM_SPLIT) {    /* (0.9.71, FLEXOR) SPLIT: the body as CLASSIC, then the tail a clean sine on
                                          * the SUB's note, faded in over half the time */
            float ms = kick_lab_ms(KLM_SPLIT, lab->amt);
            c->sp_n = (uint32_t)(ms * (float)(BRK_SR / 1000u));
            c->sp_f = (uint32_t)(ms * 0.5f * (float)(BRK_SR / 1000u)) + 1u;
            c->sp_k = 1.0f / (float)c->sp_f;
        } else if (lab->mode == KTM_DIRTY)   /* DIRTY: the drive low on the attack, rising through the tail */
            c->dr_k = 1.0f / (kick_lab_ms(KLM_RISE, lab->amt) * (float)(BRK_SR / 1000u));
        {
            float cr = kick_lab_ms(KLM_CROSS, lab->cross) * (float)(BRK_SR / 1000u);
            float fd = kick_lab_ms(KLM_FADE, lab->fade) * (float)(BRK_SR / 1000u);
            c->x0 = cr > fd * 0.5f ? (uint32_t)(cr - fd * 0.5f) : 0u;
            c->x1 = c->x0 + (uint32_t)fd + 1u;
        }
    }
    {                                    /* (0.9.45) TOK and the TAIL GATE: the kick playing's (kick_set_x2) */
        kick_x2_t x = *(const kick_x2_t *)&p_x2;
        static const uint8_t CHOPS[KICK_GATES] = {0u, 2u, 3u, 4u, 6u, 8u};   /* (a beat's chops: 1/8 .. 1/32) */
        float f0 = 400.0f * brk_exp2f_c((float)(x.tok_pitch > 100u ? 100u : x.tok_pitch) * 0.0332193f);   /* (400 ..
                                          * 4000 Hz) */
        float ms = 2.0f + 0.28f * (float)(x.tok_len > 100u ? 100u : x.tok_len);
        c->tok_lvl = (float)(x.tok > 100u ? 100u : x.tok) * 0.012f;
        c->tok_type = (uint8_t)(x.tok_type % KICK_TOK_TYPES);
        c->tok_f1 = f0 * (1.0f / 3.5f) / (float)BRK_SR;
        c->tok_fd = f0 / (float)BRK_SR - c->tok_f1;
        c->tok_pdec = decay_factor(18.0f);
        c->tok_edec = decay_factor(ms * 4.6f);
        c->tok_k = brk_clampf(6.2832f * f0 / (float)BRK_SR, 0.01f, 0.9f);
        c->g_len = 0u;
        if (x.gate && x.gate < KICK_GATES) {
            float spb = p_spb > 2000.0f ? p_spb : 2000.0f, fn;
            c->g_len = (uint32_t)(spb / (float)CHOPS[x.gate]);
            c->g_on = (uint32_t)((float)c->g_len * (0.1f + 0.008f * (float)(x.gate_len > 100u ? 100u : x.gate_len)));
            c->g_start = (uint32_t)(spb * 0.25f);
            fn = x.gate_fade ? (20.0f + 9.8f * (float)(x.gate_fade > 100u ? 100u : x.gate_fade)) * (float)(BRK_SR / 1000u) : 0.0f;
            c->g_fade_n = (uint32_t)fn;
            c->g_fade_k = fn > 0.0f ? 1.0f / fn : 0.0f;
        }
    }
}

static void kv_hit(kv_t *v)
{
    uint32_t i;
    v->declick = v->last;                /* the kick still sounding fades out under the new one */
    for (i = 0; i < KICK_LAYERS; i++) {
        v->ph[i] = 0.0f;
        v->env[i] = 1.0f;
        v->s1[i] = v->s2[i] = v->fph[i] = v->fhold[i] = 0.0f;
    }
    v->ph2 = 0.0f;
    v->pe = v->pe2 = 1.0f;
    v->lp = 0.0f;
    v->hold = 0xFFFFFFFFu;               /* (SYNTH: its HOLD, set at the first sample) */
    v->rb_t = 0u;                        /* (TEST: the REVERSE BASS, from this hit) */
    v->rb_ph = 0.0f;
    v->t = 0u;                           /* (LOAF BREAKER LAB) */
    v->pl = v->gend = v->fenv = 1.0f;
    v->f1 = v->f2 = 0.0f;                /* (the filter open at the hit: it passes it all) */
    v->tok_ph = v->tok_n1 = v->tok_n2 = 0.0f;   /* (0.9.45: TOK from the hit; the gate open) */
    v->tok_env = v->tok_pe = 1.0f;
    v->g_pos = 0u;
    v->g_s = 1.0f;
    v->sp_env = v->sp_g = 0.0f;          /* (0.9.71: SPLIT from this hit) */
}

static inline float noise(kv_t *v)
{
    v->rng = v->rng * 1664525u + 1013904223u;
    return (float)(int32_t)v->rng * (1.0f / 2147483648.0f);
}

/* a layer's own effect (on x, -1..1) */
static inline float layer_fx(kv_t *v, const kc_t *c, uint32_t i, float x)
{
    float y = x;
    switch (c->fx[i]) {
    case KFX_DRIVE:
        v->s1[i] += (soft(x * c->ga[i]) - v->s1[i]) * c->gb[i];
        y = v->s1[i];
        break;
    case KFX_CRUNCH:
        v->fph[i] += c->gb[i];
        if (v->fph[i] >= 1.0f) {
            v->fph[i] -= 1.0f;
            v->fhold[i] = (float)(int32_t)(x * c->ga[i]) / c->ga[i];
        }
        y = v->fhold[i];
        break;
    case KFX_FILTER: {                   /* (Simper's state-variable low-pass) */
        float g = c->ga[i], a1 = c->gc[i], a2 = g * a1, a3 = g * a2;
        float v3 = x - v->s2[i], v1 = a1 * v->s1[i] + a2 * v3, v2 = v->s2[i] + a2 * v->s1[i] + a3 * v3;
        v->s1[i] = 2.0f * v1 - v->s1[i];
        v->s2[i] = 2.0f * v2 - v->s2[i];
        y = v2;
        break;
    }
    case KFX_RING:
        y = x * brk_sin_turns(v->fph[i]);
        v->fph[i] += c->ga[i];
        if (v->fph[i] >= 1.0f)
            v->fph[i] -= 1.0f;
        break;
    default:
        return x;
    }
    return x + (y - x) * c->wet[i];
}

static inline float kv_tick(kv_t *v, const kc_t *c)
{
    float x[KICK_LAYERS], y = 0.0f, d, out, dirt = 1.0f;
    float clean = c->csub > 0.0f ? brk_sin_turns(v->ph[KL_SUB]) * v->env[KL_SUB] : 0.0f;   /* (CLEAN SUB: the SUB's
                                          * sine, this sample, before anything drives it) */
    uint32_t i;
    /* LOAF BREAKER LAB: the drop's shape: EXP as always, LINEAR or SNAP a ramp landing on the note */
    float pv = v->pe;
    if (c->curve) {
        float q = 1.0f - v->pl;
        pv = c->curve == 1u ? v->pl : 1.0f - q * q * q;
    }
    /* SUB: a sine dropping an octave and a half to its pitch, saturated by its tone */
    x[KL_SUB] = soft(brk_sin_turns(v->ph[KL_SUB]) * c->sub_sat) * (1.0f + c->sub_sat) / c->sub_sat;
    v->ph[KL_SUB] += c->f[KL_SUB] * v->pm * (1.0f + c->sdrop * pv);   /* (SYNTH: its DROP) */
    if (v->hold == 0xFFFFFFFFu)
        v->hold = c->hold_n;
    /* PUNCH: a sine swept down fast from far above */
    x[KL_PUNCH] = brk_sin_turns(v->ph[KL_PUNCH]);
    v->ph[KL_PUNCH] += c->f[KL_PUNCH] * v->pm * (1.0f + c->punch_sweep * v->pe2);
    /* GRIT: FM (the modulator at 1.41x), driven, noise by its tone */
    x[KL_GRIT] = soft(4.0f * (brk_sin_turns(v->ph[KL_GRIT] + c->grit_index * 0.159f * brk_sin_turns(v->ph2)) +
                              c->grit_noise * noise(v)));
    {                                    /* (LAB CLEAN: GRIT falls as the SUB does, its FM harmonic) */
        float ge = c->clean ? 1.0f + c->sdrop * pv : 1.0f + pv;
        v->ph[KL_GRIT] += c->f[KL_GRIT] * v->pm * ge;
        v->ph2 += c->f[KL_GRIT] * v->pm * c->grat * ge;
    }
    /* CLICK: noise high-passed at its pitch, and a click ringing there; tone between them */
    {
        float n = noise(v), ring = brk_sin_turns(v->ph[KL_CLICK]);
        v->lp += (n - v->lp) * c->click_k;
        x[KL_CLICK] = (n - v->lp) * c->click_noise + ring * (1.0f - c->click_noise);
        v->ph[KL_CLICK] += c->f[KL_CLICK];
    }
    {                                    /* (LAB CLEAN: the layers against the SUB's level: the distortion works on a
                                          * steady sound, the SUB's fall put on after it) */
    float ie = c->clean ? 1.0f / (v->env[KL_SUB] > 1e-4f ? v->env[KL_SUB] : 1e-4f) : 0.0f;
    for (i = 0; i < KICK_LAYERS; i++) {
        float e = v->env[i];
        if (c->clean) {
            e *= ie;
            e = e < 4.0f ? e : 4.0f;
        }
        if (v->ph[i] >= 1.0f)
            v->ph[i] -= (float)(uint32_t)v->ph[i];
        y += layer_fx(v, c, i, x[i] * e) * c->lvl[i];
        if (i == KL_SUB && v->hold)      /* (SYNTH: the SUB held at full) */
            v->hold--;
        else
            v->env[i] *= c->dec[i];
    }
    }
    if (v->ph2 >= 1.0f)
        v->ph2 -= (float)(uint32_t)v->ph2;
    if (c->curve)
        v->pl = v->pl > c->pl_step ? v->pl - c->pl_step : 0.0f;
    v->pe *= c->pe_dec;
    v->pe2 *= c->pe2_dec;
    if (c->rb_lvl > 0.0f && v->rb_t < c->rb_e) {   /* (TEST) the REVERSE BASS: the sub's note swelling up to the
                                          * next beat, then gone (the next kick takes over) */
        if (v->rb_t >= c->rb_s) {
            float e = (float)(v->rb_t - c->rb_s) / (float)(c->rb_e - c->rb_s), p = e;
            if (c->rb_k > 1.5f)
                p *= e;
            if (c->rb_k > 2.5f)
                p *= e;
            if (c->rb_k > 3.5f)
                p *= e;
            if (c->rb_e - v->rb_t < 66u)  /* (the last 1.5 ms: down to nothing, no click) */
                p *= (float)(c->rb_e - v->rb_t) * (1.0f / 66.0f);
            y += brk_sin_turns(v->rb_ph) * p * c->rb_lvl;
            v->rb_ph += c->f[KL_SUB] * v->pm;
            if (v->rb_ph >= 1.0f)
                v->rb_ph -= 1.0f;
        }
        v->rb_t++;
    }
    if (c->tok_lvl > 0.0f && v->tok_env > 1e-4f) {   /* (0.9.45) TOK: the short pitched attack, into the clip */
        float s;
        if (c->tok_type == 1u) {         /* NOISE: high-passed, then low-passed, in a band */
            float n = noise(v);
            v->tok_n1 += (n - v->tok_n1) * c->tok_k;
            v->tok_n2 += ((n - v->tok_n1) - v->tok_n2) * (c->tok_k * 2.5f < 0.9f ? c->tok_k * 2.5f : 0.9f);
            s = v->tok_n2 * 3.0f;
        } else {
            s = c->tok_type == 2u ? brk_sin_turns(v->tok_ph) : (v->tok_ph < 0.5f ? 1.0f : -1.0f);
            v->tok_n2 += (s - v->tok_n2) * 0.6f;   /* (the square's edges rounded, ~7 kHz) */
            s = v->tok_n2;
            v->tok_ph += c->tok_f1 + c->tok_fd * v->tok_pe;
            if (v->tok_ph >= 1.0f)
                v->tok_ph -= 1.0f;
            v->tok_pe *= c->tok_pdec;
        }
        y += s * v->tok_env * c->tok_lvl;
        v->tok_env *= c->tok_edec;
    }
    /* the sum: a drive into a clip, soft and then (INTENSITY over half) hard: the distorted kick */
    d = y * c->g;
    if (c->dr_k > 0.0f) {                /* (0.9.71) TAIL MODE DIRTY: 0.35x the drive on the hit, up to 3x along the tail */
        float r = (float)v->t * c->dr_k;
        r = r < 1.0f ? r * (2.0f - r) : 1.0f;
        dirt = 0.35f + 2.65f * r;
        d *= dirt;
    }
    if (c->tdrive > 0.0f)                /* (LAB: TAIL DRIVE, after the attack) */
        d *= 1.0f + c->tdrive * (1.0f - v->pe2);
    if (c->asym > 0.0f)                  /* (SYNTH: ASYM, uneven: one side pushed harder) */
        d += c->asym * d * brk_absf(d);
    out = (soft(d) * (1.0f - c->hard) + brk_clampf(d, -1.0f, 1.0f) * c->hard) * c->make;
    if (c->asym > 0.0f) {                /* (its offset taken off) */
        v->dc += (out - v->dc) * 0.0005f;
        out -= v->dc;
    }
    if (c->tmix > 0.0f) {                /* LOAF BREAKER: TERROR: the sub (below ~110 Hz) kept, the rest driven into a
                                          * triangle fold (it folds back instead of flattening: the scream), then
                                          * clipped and its fizz rounded */
        float h, t, z, a = brk_absf(out);
        v->tenv += (a - v->tenv) * (a > v->tenv ? 0.05f : c->trel);   /* (the kick's level: fast up, ~20 ms down;
                                          * TAIL: slower or faster) */
        v->tlo += (out - v->tlo) * c->tsub;
        h = (out - v->tlo) * c->tg * dirt;   /* (DIRTY: the DISTO rising too) */
        t = h * 0.25f + 0.25f;
        t -= (float)(int32_t)t;
        if (t < 0.0f)
            t += 1.0f;
        z = 1.0f - 4.0f * brk_absf(t - 0.5f);   /* (the fold: h within +-1 untouched, beyond it back and forth) */
        if (c->ttype == 1u)              /* (TEST) CLIP: squared off instead */
            z = brk_clampf(h, -1.0f, 1.0f);
        else if (c->ttype == 2u)         /* CRUSH: folded, then in eight steps */
            z = (float)(int32_t)(z * 4.0f) * 0.25f;
        else if (c->ttype == 6u)         /* LOAF BREAKER (0.255, FLEXOR: more types): SINE, folded round a sine (it
                                          * sings as the drive goes up, metallic) */
            z = brk_sin_turns(h * 0.25f);
        else if (c->ttype == 7u) {       /* WRAP: past the edge, back in from the other side (digital, harsh) */
            float w = h * 0.5f + 0.5f;
            w -= (float)(int32_t)w;
            if (w < 0.0f)
                w += 1.0f;
            z = 2.0f * w - 1.0f;
        } else if (c->ttype == 8u)       /* FUZZ: clipped to all but a square (a thick buzz) */
            z = soft(h * 6.0f) * 1.15f;
        else if (c->ttype >= 3u) {       /* LOAF BREAKER: RECT, rectified (an octave up, hollow and metallic); TUBE,
                                          * one side pushed harder than the other (even harmonics, warm and fat); HALF,
                                          * one side alone, pushed (dirty, hollow, the low end kept); their offset
                                          * taken off */
            float s = soft(h);
            z = c->ttype == 3u ? 2.0f * brk_absf(s) : c->ttype == 5u ? (h > 0.0f ? 2.0f * soft(h * 1.5f) : 0.0f)
                : (h > 0.0f ? soft(h * 1.8f) : 0.6f * soft(h * 0.7f));
            v->rdc += (z - v->rdc) * 0.002f;
            z -= v->rdc;
        }
        if (c->sgain > 0.0f) {           /* SCREECH: its band, resonant, on top */
            float v3 = z - v->sc2, v1 = c->sa1 * v->sc1 + c->sa2 * v3, v2 = v->sc2 + c->sa2 * v->sc1 + c->sa3 * v3;
            v->sc1 = 2.0f * v1 - v->sc1;
            v->sc2 = 2.0f * v2 - v->sc2;
            z = brk_clampf(z + v1 * c->sgain, -1.5f, 1.5f);
        }
        v->tlp += (z - v->tlp) * c->tfz;
        a = v->tenv * c->tsus;              /* (the fold follows the kick down: its punch and its tail kept, not a wall) */
        z = brk_clampf(v->tlo * 1.2f + v->tlp * 0.85f * (a < 1.0f ? a : 1.0f), -1.0f, 1.0f);
        out += (z - out) * c->tmix;
    }
    if (c->t2mix > 0.0f) {               /* LOAF BREAKER: TERROR 2: the whole kick driven into a sine folder (round
                                          * and round: the screech), its mids pushed, clipped hard; the sub back under */
        float a = brk_absf(out), z, m;
        v->t2env += (a - v->t2env) * (a > v->t2env ? 0.05f : 0.0012f);
        v->t2lo += (out - v->t2lo) * c->t2sub;   /* (~110 Hz) */
        z = brk_sin_turns(out * c->t2g * 0.25f);   /* (|x| up to 1: a sine; past it, folded back and forth) */
        v->t2lp += (z - v->t2lp) * c->t2scr;   /* (~600 Hz: what's above it pushed, the scream) */
        z = brk_clampf((z + c->t2push * (z - v->t2lp)) * 1.3f, -1.0f, 1.0f);
        m = v->t2env * 2.5f;
        z = brk_clampf(v->t2lo * 1.3f + z * 0.8f * (m < 1.0f ? m : 1.0f), -1.0f, 1.0f);
        out += (z - out) * c->t2mix;
    }
    if (c->clean) {                      /* LOAF BREAKER LAB CLEAN: the SUB's fall, as loud as the clip made it */
        float et = v->env[KL_SUB] > 1e-4f ? v->env[KL_SUB] : 1e-4f, ge = et * c->g, eg = soft(ge) / soft(c->g);
        out *= eg * (1.0f - c->hard) + (ge < 1.0f ? ge : 1.0f) * c->hard;
    }
    if (c->filt) {                       /* LAB: the TAIL FILTER, closing as the kick goes */
        float a = c->fa_end + (1.0f - c->fa_end) * v->fenv;
        v->f1 += (out - v->f1) * a;
        v->f2 += (v->f1 - v->f2) * a;
        out = v->f2;
        v->fenv *= c->f_dec;
    }
    if (c->rum > 0.0f && rum_buf && c->rlen) {   /* (SYNTH: RUMBLE, the kick under ~150 Hz into a feedback line, its
                                          * echoes low-passed again: the long low roll after each hit) */
        float tap = (float)rum_buf[rum_pos] * (1.0f / 32767.0f), w;
        v->rlp += (out - v->rlp) * 0.02f;
        w = brk_clampf(v->rlp + tap * c->rfb, -1.0f, 1.0f);
        rum_buf[rum_pos] = (int16_t)(w * 32767.0f);
        if (++rum_pos >= c->rlen)
            rum_pos = 0u;
        v->rout += (tap - v->rout) * 0.06f;
        v->rlvl = brk_absf(v->rout) > v->rlvl ? brk_absf(v->rout) : v->rlvl * 0.9995f;
        out = brk_clampf(out + v->rout * c->rum * 1.5f, -1.2f, 1.2f);
    }
    if (c->csub > 0.0f) {                /* LOAF BREAKER: CLEAN SUB: the distorted kick's lows (~90 Hz down) out, a pure
                                          * sine at the SUB's note in: the round bass under the dirt */
        v->clp += (out - v->clp) * 0.013f;
        out = brk_clampf(out - v->clp * c->csub + clean * 0.9f * c->csub, -1.2f, 1.2f);
    }
    if (c->sp_n) {                       /* (0.9.71) TAIL MODE SPLIT: from sp_n, the kick faded into a clean sine on the
                                          * SUB's phase, as loud as the kick was there, falling as the SUB does */
        float a = brk_absf(out);
        if (v->t < c->sp_n) {
            v->sp_env += (a - v->sp_env) * (a > v->sp_env ? 0.2f : 0.0015f);   /* (~15 ms down: a cycle's peak held) */
        } else {
            float e = v->env[KL_SUB] > 1e-4f ? v->env[KL_SUB] : 1e-4f, x, sn;
            uint32_t k = v->t - c->sp_n;
            if (k == 0u)
                v->sp_g = v->sp_env / e;
            x = k >= c->sp_f ? 1.0f : (float)k * c->sp_k;
            sn = brk_sin_turns(v->ph[KL_SUB]) * v->env[KL_SUB] * v->sp_g;
            out += (brk_clampf(sn, -1.2f, 1.2f) - out) * x;
        }
    }
    if (c->end_n && v->t >= c->end_n) {  /* LAB: END, the kick cut (in ~5 ms) */
        v->gend *= 0.979f;
        if (v->gend < 1e-4f) {
            v->gend = 0.0f;
            for (i = 0; i < KICK_LAYERS; i++)
                v->env[i] = 0.0f;
        }
        out *= v->gend;
    }
    if (c->g_len && v->t >= c->g_start) {   /* (0.9.45) the TAIL GATE: from the second 16th, chopped in time */
        float on = v->g_pos < c->g_on ? 1.0f : 0.0f, tg = on;
        if (++v->g_pos >= c->g_len)
            v->g_pos = 0u;
        if (c->g_fade_n) {               /* (coming in little by little) */
            uint32_t e = v->t - c->g_start;
            float k = e >= c->g_fade_n ? 1.0f : (float)e * c->g_fade_k;
            tg = 1.0f - k * (1.0f - on);
        }
        v->g_s += (tg - v->g_s) * 0.06f;
        out *= v->g_s;
    }
    v->t++;
    out *= c->vol;
    v->last = out;
    out += v->declick;
    v->declick *= 0.985f;
    return out;
}

BRK_COLD void kick_init(void)
{
    uint32_t i;
    for (i = 0; i < KICK_LAYERS; i++)
        kv.env[i] = 0.0f;
    kv.declick = kv.last = 0.0f;
    kv.rng = 0x5eed1u;
    kv.tlo = kv.tlp = kv.tenv = 0.0f;
    kv.t2lo = kv.t2lp = kv.t2env = 0.0f;
    kv.sc1 = kv.sc2 = 0.0f;
    kv.rlp = kv.rout = kv.rlvl = kv.dc = 0.0f;
    kv.hold = 0u;
    kv.pm = kv.pm_t = 1.0f;
    kv.rb_t = 0xFFFFFFFFu;
    kv.t = 0u;
    kv.pl = kv.gend = kv.fenv = 1.0f;
    kv.f1 = kv.f2 = 0.0f;
    kv2 = kv;                            /* (STACK's attack: the same, silent) */
    ks_last = 0.0f;
    ks_on = 0u;
    hits_seen = p_hits;
}

BRK_COLD void kick_set(const kick_t *k) { *(kick_t *)&p_k = *k; }
void kick_hit(void) { p_hits++; }
void kick_speed(const float *sp) { ks_sp = sp; }
static const float *ks_pm;               /* (crossbreed) the next render's hits' pitches (kick_pitches), 0 none */
void kick_pitches(const float *pm) { ks_pm = pm; }

/* LOAF BREAKER LAB: STACK, the attack's voice over the kick's own until the hand-over, then the kick alone (SOLO:
 * either alone) */
static uint8_t st_on;
static inline float kk_tick(const kc_t *c, const kc_t *c2)
{
    uint32_t t = kv.t;
    float b = kv_tick(&kv, c), a, x;
    if (!st_on || t > c->x1 || kv2.t != t)   /* (past the hand-over, or not hit with the kick: STACK came on since) */
        return p_solo == 1u ? 0.0f : b;
    kv2.pm = kv.pm;
    a = kv_tick(&kv2, c2);
    x = t <= c->x0 ? 0.0f : (float)(t - c->x0) / (float)(c->x1 - c->x0);
    if (t == c->x1)                      /* (handed over: its voice left, nothing to fade in at the next hit) */
        kv2.last = kv2.declick = 0.0f;
    return (p_solo == 2u ? 0.0f : a * (1.0f - x)) + (p_solo == 1u ? 0.0f : b * x);
}
void kick_render(float *out, uint32_t n, const uint16_t *at, uint32_t nat)
{
    kick_t k = *(const kick_t *)&p_k;
    kick_lab_t lab = *(const kick_lab_t *)&p_lab;
    kc_t c, c2;
    uint32_t i, j = 0;
    st_on = p_aon && lab.src;
    if (p_hits != hits_seen) {           /* a MIDI note: now */
        hits_seen = p_hits;
        kv_hit(&kv);
        kv.pm = kv.pm_t = 1.0f;
        if (st_on)
            kv_hit(&kv2);
    } else if (!nat && kv.env[KL_SUB] < 3e-5f && kv.env[KL_PUNCH] < 3e-5f && kv.env[KL_GRIT] < 3e-5f &&
               kv.env[KL_CLICK] < 3e-5f && brk_absf(kv.declick) < 1e-5f && kv.rlvl < 1e-4f &&
               (p_rbl <= 0.0f || kv.rb_t >= (uint32_t)p_spb)) {   /* (TEST: and no REVERSE BASS to come) */   /* (and the rumble
                                          * died away) */
        for (i = 0; i < n; i++)          /* gone quiet, nothing coming: no work */
            out[i] = 0.0f;
        kv.last = ks_last = 0.0f;
        ks_on = 0u;
        return;
    }
    kc_make(&k, &c, p_syn, &lab);
    if (st_on) {                         /* (the attack's settings: no rumble, reverse bass or clean sub of its own) */
        kick_t ak = *(const kick_t *)&p_ak;
        kick_lab_t al = *(const kick_lab_t *)&p_alab;
        kc_make(&ak, &c2, p_asyn, &al);
        c2.rum = c2.rb_lvl = c2.csub = 0.0f;
        c2.tok_lvl = 0.0f;               /* (0.9.45: TOK and the gate the tail kick's alone) */
        c2.g_len = 0u;
        c2.vol = c.vol;                  /* (at the kick's volume) */
    } else
        c2 = c;
    if (ks_sp && !ks_on) {               /* the vinyl begins: from the kick as it is */
        ks_a = ks_b = ks_last;
        ks_ph = 0.0f;
    }
    ks_on = ks_sp != 0;
    {
    float gk = p_glide > 0.5f ? 1.0f / (p_glide * 0.001f * (float)BRK_SR) : 1.0f;   /* (TEST) GLIDE's step a sample */
    for (i = 0; i < n; i++) {
        while (j < nat && at[j] <= i) {  /* the steps' kicks, on their samples */
            float np = ks_pm ? ks_pm[j] : 1.0f;
            kv_hit(&kv);
            if (st_on)
                kv_hit(&kv2);
            kv.pm_t = np;                /* (crossbreed: this step's note; TEST: GLIDE, slid to from the last) */
            if (gk >= 1.0f)
                kv.pm = np;
            j++;
        }
        if (gk < 1.0f)
            kv.pm += (kv.pm_t - kv.pm) * gk;
        if (!ks_sp)
            out[i] = kk_tick(&c, &c2);
        else {                           /* the vinyl: the kick's own time runs at the speed, read between */
            float s = ks_sp[i];
            ks_ph += s;
            if (ks_ph >= 1.0f) {
                ks_ph -= 1.0f;
                ks_a = ks_b;
                ks_b = kk_tick(&c, &c2);
            }
            out[i] = (ks_a + (ks_b - ks_a) * ks_ph) * (s < 0.02f ? s * 50.0f : 1.0f);
        }
    }
    }
    ks_last = out[n - 1u];
}

BRK_COLD void kick_shape(const kick_t *k, uint8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    kc_t c;
    kv_t v;
    uint32_t i, total = ms * (BRK_SR / 1000u), per = total / (n ? n : 1u);
    {
        kick_lab_t lab = *(const kick_lab_t *)&p_lab;
        kc_make(k, &c, p_syn, &lab);
    }
    c.rum = 0.0f;                        /* (the picture: no rumble, its line is the kick's playing) */
    c.rb_lvl = 0.0f;                     /* (nor the REVERSE BASS) */
    v.rng = 0x5eed1u;
    v.sc1 = v.sc2 = 0.0f;
    v.rlp = v.rout = v.rlvl = v.dc = 0.0f;
    v.tlo = v.tlp = v.tenv = 0.0f;
    v.t2lo = v.t2lp = v.t2env = 0.0f;
    v.pm = 1.0f;
    v.last = v.declick = 0.0f;
    kv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t s;
        float pk = 0.0f;
        for (s = 0; s < per; s++) {
            float a = brk_absf(kv_tick(&v, &c));
            if (a > pk)
                pk = a;
        }
        px[i] = (uint8_t)(brk_clampf(pk, 0.0f, 1.0f) * (float)height + 0.5f);
    }
}

BRK_COLD void kick_rgb(const kick_t *k, uint8_t (*col)[4], uint32_t n, uint32_t ms)
{
    static const float G[4] = {300.0f, 380.0f, 520.0f, 255.0f};   /* (app.c's scope: the highs up, they are quieter) */
    kc_t c;
    kv_t v;
    uint32_t i, j, total = ms * (BRK_SR / 1000u), per = total / (n ? n : 1u);
    float l1 = 0.0f, l2 = 0.0f, e[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    {
        kick_lab_t lab = *(const kick_lab_t *)&p_lab;
        kc_make(k, &c, p_syn, &lab);
    }
    c.rum = 0.0f;
    c.rb_lvl = 0.0f;
    v.rng = 0x5eed1u;
    v.tlo = v.tlp = v.tenv = 0.0f;
    v.t2lo = v.t2lp = v.t2env = 0.0f;
    v.sc1 = v.sc2 = 0.0f;
    v.rlp = v.rout = v.rlvl = v.dc = 0.0f;
    v.pm = 1.0f;
    v.last = v.declick = 0.0f;
    kv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t s;
        float pk[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (s = 0; s < per; s++) {
            float x = kv_tick(&v, &c), b[4];
            l1 += (x - l1) * 0.0285f;    /* (~200 Hz) */
            l2 += (x - l2) * 0.356f;     /* (~2.5 kHz) */
            b[0] = brk_absf(l1);
            b[1] = brk_absf(l2 - l1);
            b[2] = brk_absf(x - l2);
            b[3] = brk_absf(x);
            for (j = 0; j < 4u; j++) {   /* (each band followed, ~15 ms down: smooth as rekordbox, not a cycle's
                                          * peaks column by column) */
                e[j] = b[j] > e[j] ? b[j] : e[j] * 0.9985f;
                pk[j] = e[j] > pk[j] ? e[j] : pk[j];
            }
        }
        for (j = 0; j < 4u; j++)
            col[i][j] = (uint8_t)brk_clampf(pk[j] * G[j], 0.0f, 255.0f);
    }
}

BRK_COLD void kick_wave(const kick_t *k, int8_t *px, uint32_t n, uint32_t ms, uint32_t height)
{
    kc_t c;
    kv_t v;
    uint32_t i, total = ms * (BRK_SR / 1000u), per = total / (n ? n : 1u);
    {
        kick_lab_t lab = *(const kick_lab_t *)&p_lab;
        kc_make(k, &c, p_syn, &lab);
    }
    c.rum = 0.0f;
    c.rb_lvl = 0.0f;
    v.rng = 0x5eed1u;
    v.tlo = v.tlp = v.tenv = 0.0f;
    v.t2lo = v.t2lp = v.t2env = 0.0f;
    v.sc1 = v.sc2 = 0.0f;
    v.rlp = v.rout = v.rlvl = v.dc = 0.0f;
    v.pm = 1.0f;
    v.last = v.declick = 0.0f;
    kv_hit(&v);
    v.declick = 0.0f;
    for (i = 0; i < n; i++) {
        uint32_t s;
        float best = 0.0f;
        for (s = 0; s < per; s++) {
            float x = kv_tick(&v, &c);
            if (brk_absf(x) > brk_absf(best))
                best = x;
        }
        px[i] = (int8_t)(brk_clampf(best, -1.0f, 1.0f) * (float)height);
    }
}

/* the built-in kicks: per layer level, pitch, decay, tone; then intensity */
static const float PRESET[KICK_PRESETS][KICK_LAYERS * 4 + 1] = {
    {0.85f, 0.50f, 0.62f, 0.15f, 0.75f, 0.55f, 0.35f, 0.55f, 0.00f, 0.30f, 0.30f, 0.30f, 0.25f, 0.25f, 0.25f, 0.20f,
     0.20f},                             /* 909 (0.9.39, FLEXOR: rounder, the SUB 614 ms, a pure sine: app.c kt_extra) */
    {0.90f, 0.25f, 0.58f, 0.30f, 0.60f, 0.40f, 0.30f, 0.35f, 0.15f, 0.25f, 0.25f, 0.20f, 0.30f, 0.55f, 0.20f, 0.50f,
     0.35f},                             /* TECHNO (0.9.39: deeper, 39 Hz, longer, the RUMBLE on) */
    {0.90f, 0.55f, 0.52f, 0.85f, 0.85f, 0.60f, 0.50f, 0.75f, 0.20f, 0.40f, 0.30f, 0.40f, 0.40f, 0.55f, 0.20f, 0.30f,
     0.80f},                             /* EARLY HC (0.9.39: more body, the SUB saturated, DISTO HALF 25, ASYM) */
    {0.90f, 0.30f, 0.60f, 0.60f, 0.70f, 0.45f, 0.45f, 0.70f, 0.55f, 0.30f, 0.55f, 0.70f, 0.35f, 0.60f, 0.20f, 0.50f,
     0.85f},                             /* ROTTERDAM: the long distorted tail */
    {0.85f, 0.25f, 0.70f, 0.55f, 0.60f, 0.50f, 0.35f, 0.50f, 0.55f, 0.45f, 0.55f, 0.55f, 0.40f, 0.45f, 0.25f, 0.70f,
     0.75f},                             /* INDUSTRIAL (0.9.39: darker: GRIT through RING MOD, DISTO TUBE 25) */
    {0.85f, 0.35f, 0.55f, 0.80f, 0.85f, 0.50f, 0.40f, 0.80f, 0.70f, 0.40f, 0.50f, 0.80f, 0.40f, 0.55f, 0.25f, 0.45f,
     1.00f},                             /* DISTORTO: everything into the clip (out of PRESET's round, 0.9.39) */
    {0.90f, 0.30f, 0.92f, 0.60f, 0.70f, 0.45f, 0.45f, 0.70f, 0.00f, 0.30f, 0.55f, 0.70f, 0.35f, 0.60f, 0.20f, 0.50f,
     0.69f},                             /* TERROR (FLEXOR): ROTTERDAM, the SUB to 1279 ms, no GRIT, DRIVE 69 % */
    {0.90f, 0.30f, 0.70f, 0.60f, 0.70f, 0.45f, 0.45f, 0.70f, 0.00f, 0.30f, 0.55f, 0.70f, 0.35f, 0.60f, 0.20f, 0.50f,
     0.50f},                             /* SPINE (FLEXOR): TERROR, the SUB to 766 ms, DRIVE 50 %, DISTO RECT below */
    {0.85f, 0.50f, 0.65f, 0.35f, 0.80f, 0.55f, 0.40f, 0.65f, 0.10f, 0.30f, 0.30f, 0.30f, 0.30f, 0.30f, 0.25f, 0.25f,
     0.75f},                             /* 909 DRIVE (0.9.39, FLEXOR): the 909 driven, DISTO TUBE 15 */
    {0.90f, 0.45f, 0.66f, 0.70f, 0.80f, 0.50f, 0.45f, 0.75f, 0.25f, 0.30f, 0.40f, 0.40f, 0.35f, 0.50f, 0.20f, 0.40f,
     0.95f},                             /* GABBER (0.9.39, FLEXOR): DRIVE 95 %, DISTO FOLD 25, the SUB 690 ms */
    {0.90f, 0.35f, 0.85f, 0.55f, 0.75f, 0.50f, 0.40f, 0.70f, 0.15f, 0.30f, 0.50f, 0.40f, 0.30f, 0.55f, 0.20f, 0.40f,
     0.85f},                             /* MILLENNIUM (0.9.39, FLEXOR): a long tuned tail, the SUB 1100 ms */
    {0.95f, 0.12f, 0.95f, 0.60f, 0.65f, 0.35f, 0.50f, 0.50f, 0.20f, 0.20f, 0.60f, 0.30f, 0.25f, 0.30f, 0.20f, 0.30f,
     0.80f},                             /* DOOMCORE (0.9.39, FLEXOR): 34 Hz, the SUB 1360 ms, DISTO TUBE 20 */
};
/* (their DISTO: amount, D+ TYPE; the SUB through DRIVE (fa 0.5); GRIT's effect, its fa and mix in %) */
static const uint8_t PRESET_X[KICK_PRESETS][6] = {
    {0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {25, 5, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}, {25, 4, 0, KFX_RING, 35, 50},
    {0, 0, 0, 0, 0, 0}, {30, 0, 1, 0, 0, 0}, {30, 3, 1, 0, 0, 0}, {15, 4, 0, 0, 0, 0}, {25, 0, 0, 0, 0, 0},
    {20, 0, 0, 0, 0, 0}, {20, 4, 0, 0, 0, 0},
};

BRK_COLD void kick_preset(kick_t *k, uint32_t i)
{
    const float *p = PRESET[i % KICK_PRESETS];
    uint32_t l;
    for (l = 0; l < KICK_LAYERS; l++) {
        k->l[l].level = p[l * 4u];
        k->l[l].pitch = p[l * 4u + 1u];
        k->l[l].decay = p[l * 4u + 2u];
        k->l[l].tone = p[l * 4u + 3u];
        k->l[l].fx = KFX_OFF;
        k->l[l].rsv[0] = k->l[l].rsv[1] = k->l[l].rsv[2] = 0u;
        k->l[l].fa = 0.5f;
        k->l[l].fb = 0.5f;
        k->l[l].wet = 1.0f;
    }
    k->intensity = p[KICK_LAYERS * 4u];
    {                                    /* (their DISTO, the SUB's DRIVE (TERROR, SPINE), GRIT's effect) */
        const uint8_t *x = PRESET_X[i % KICK_PRESETS];
        KICK_TERROR(k) = x[0];
        KICK_T_TYPE(k) = x[1];
        if (x[2])
            k->l[KL_SUB].fx = KFX_DRIVE;
        if (x[3]) {
            k->l[KL_GRIT].fx = x[3];
            k->l[KL_GRIT].fa = (float)x[4] * 0.01f;
            k->l[KL_GRIT].wet = (float)x[5] * 0.01f;
        }
    }
}

BRK_COLD void kick_defaults(kick_t *k)
{
    kick_preset(k, KICK_PRESET_ROTTERDAM);   /* ROTTERDAM: CHOPPAH is for the hard stuff */
    k->volume = 0.8f;
}
