/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH DRUMS: two synthesised kits of its own for the drum track (KIT: FM DRUM, C64 DRUM), every sound edited on
 * the device (EDIT with the drum track selected: KIT, the model's three pages, DRUM FX), each sound with its own
 * saturation (SOFT, HARD, FOLD), delay send and reverb send. Included by drums.c after drum_synth.c; the voices run in
 * the audio ISR, on the drum voices' memory (drums.kgv: a union with dsv_t).
 *   FM   a sine carrier phase-modulated by a sine at RATIO x its pitch (with FEEDBACK on itself); the modulation INDEX
 *        falls over IDEC (bright attack, round body); a pitch drop (BEND semitones over BTIME); the body's DECAY;
 *        noise (NOISE, NDEC) through its COLOUR (0 dark .. 64 white .. 127 bright)
 *   C64  a voice of the C64's SID run on 50 Hz frames, as the C64 drum routines: the attack WAVE for FRAMES frames
 *        (noise, pulse, saw, triangle or none), then the body WAVE, its pitch moving SWEEP each frame and its pulse
 *        width PWM; the SID's decay rates; its filter (LP, BP, HP: CUT, RES); 12-bit waves, its 23-bit noise
 * The per-sound FX: SAT (OFF, SOFT tanh, HARD clip, FOLD a wavefolder) with its DRIVE, then the sound's LEVEL; the
 * sends DLY and REV go to the tempo delay and the reverb (drums.c drums_mix), the drum track's global sends aside. */

enum { KGD_FM, KGD_C64, KGD_NK };
#define KGD_NP 12u
typedef struct {
    uint8_t p[KGD_NP];                             /* the model's (FM_* or C6_*) */
    uint8_t sat, drv, dly, rev;                    /* KS_*, 0..127 */
} kgd_snd_t;
enum { KS_OFF, KS_SOFT, KS_HARD, KS_FOLD };
enum { FM_PIT, FM_RAT, FM_IDX, FM_IDEC, FM_FB, FM_BEND, FM_BT, FM_DEC, FM_NZ, FM_NDEC, FM_NCOL, FM_LVL };
enum { C6_PIT, C6_W1, C6_FR, C6_W2, C6_SWP, C6_PW, C6_PWM, C6_DEC, C6_FLT, C6_CUT, C6_RES, C6_LVL };
enum { CW_NOISE, CW_PULSE, CW_SAW, CW_TRI, CW_OFF };
/* each value's top (the editor's ranges; kgd_fix keeps flash and the editor's values in them) */
static const uint8_t KGD_MAX[KGD_NK][KGD_NP] = {{108, 15, 127, 127, 127, 48, 127, 127, 127, 127, 127, 127},
                                                {120, 4, 15, 3, 127, 127, 127, 15, 3, 127, 15, 127}};
static const uint8_t KGD_MIN[KGD_NK][KGD_NP] = {{24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
/* FM: the modulator's ratio, Q8 (harmonic, then the bells' and the metals' inharmonic ones) */
static const uint16_t KGD_RATIO[16] = {128, 256, 361, 384, 512, 640, 707, 768, 896, 1024, 1280, 1382, 1536, 1792, 2048, 2867};
/* C64: the SID's decay rates (6 ms .. 24 s) as DECAY_K steps (5 ms .. 4 s to -60 dB) */
static const uint8_t KGD_SIDK[16] = {4, 30, 43, 51, 59, 67, 70, 73, 78, 95, 108, 117, 121, 127, 127, 127};

/* the factory sounds, lane by lane (drums.c LANE_NAME: kick, kick 2, snare, clap, hat, open hat, pedal, rim, snare 2,
 * low tom, hi tom, crash, ride, shaker, conga, cowbell) */
#define KS(a, b, c, d, e, f, g, h, i, j, k, l, s, dr, dl, rv) {{a, b, c, d, e, f, g, h, i, j, k, l}, s, dr, dl, rv}
static const kgd_snd_t KGD_DEF[KGD_NK][DRUM_LANES] = {
    {   /* FM: PIT RAT IDX IDEC FB BEND BT DEC NZ NDEC NCOL LVL | SAT DRV DLY REV */
        KS(33, 1, 40, 22, 0, 24, 30, 76, 12, 6, 100, 112, KS_SOFT, 24, 0, 0),
        KS(29, 0, 70, 26, 20, 36, 36, 84, 0, 0, 64, 108, KS_HARD, 40, 0, 0),
        KS(50, 2, 60, 34, 30, 12, 20, 52, 100, 62, 90, 104, KS_OFF, 0, 0, 24),
        KS(60, 6, 30, 10, 60, 0, 0, 12, 120, 58, 82, 100, KS_OFF, 0, 0, 44),
        KS(84, 11, 110, 30, 100, 0, 0, 22, 90, 22, 120, 84, KS_OFF, 0, 0, 0),
        KS(84, 11, 110, 70, 100, 0, 0, 62, 90, 60, 120, 80, KS_OFF, 0, 12, 10),
        KS(84, 11, 110, 20, 100, 0, 0, 12, 90, 14, 120, 76, KS_OFF, 0, 0, 0),
        KS(79, 7, 50, 8, 0, 0, 0, 14, 30, 6, 110, 100, KS_OFF, 0, 0, 10),
        KS(55, 4, 80, 26, 60, 7, 15, 42, 110, 54, 70, 100, KS_FOLD, 30, 0, 16),
        KS(43, 1, 30, 30, 0, 12, 45, 70, 10, 10, 60, 106, KS_OFF, 0, 0, 12),
        KS(50, 1, 30, 30, 0, 12, 45, 66, 10, 10, 60, 104, KS_OFF, 0, 0, 12),
        KS(72, 15, 127, 90, 127, 0, 0, 96, 100, 95, 115, 80, KS_OFF, 0, 0, 40),
        KS(76, 12, 90, 86, 80, 0, 0, 92, 40, 80, 120, 80, KS_OFF, 0, 0, 20),
        KS(90, 9, 60, 10, 127, 0, 0, 16, 100, 26, 127, 74, KS_OFF, 0, 0, 0),
        KS(62, 3, 25, 16, 0, 5, 15, 46, 0, 0, 64, 100, KS_OFF, 0, 0, 8),
        KS(77, 2, 45, 40, 10, 0, 0, 46, 0, 0, 64, 90, KS_SOFT, 30, 20, 0),
    },
    {   /* C64: PIT W1 FR W2 SWP PW PWM DEC FLT CUT RES LVL | SAT DRV DLY REV */
        KS(57, CW_NOISE, 1, CW_PULSE, 48, 64, 64, 7, 1, 92, 4, 110, KS_HARD, 20, 0, 0),
        KS(52, CW_NOISE, 1, CW_TRI, 52, 64, 64, 8, 0, 127, 0, 112, KS_SOFT, 30, 0, 0),
        KS(70, CW_PULSE, 1, CW_NOISE, 60, 40, 64, 8, 0, 127, 0, 100, KS_OFF, 0, 0, 16),
        KS(84, CW_NOISE, 0, CW_NOISE, 64, 64, 64, 5, 2, 90, 8, 100, KS_OFF, 0, 0, 30),
        KS(108, CW_NOISE, 0, CW_NOISE, 64, 64, 64, 2, 3, 100, 0, 112, KS_OFF, 0, 0, 0),
        KS(108, CW_NOISE, 0, CW_NOISE, 64, 64, 64, 8, 3, 96, 0, 100, KS_OFF, 0, 10, 0),
        KS(108, CW_NOISE, 0, CW_NOISE, 64, 64, 64, 1, 3, 104, 0, 112, KS_OFF, 0, 0, 0),
        KS(84, CW_PULSE, 1, CW_PULSE, 56, 20, 64, 2, 0, 127, 0, 90, KS_OFF, 0, 0, 0),
        KS(64, CW_NOISE, 1, CW_SAW, 58, 64, 64, 7, 1, 100, 6, 100, KS_FOLD, 24, 0, 10),
        KS(52, CW_NOISE, 1, CW_TRI, 60, 64, 64, 8, 0, 127, 0, 104, KS_OFF, 0, 0, 10),
        KS(60, CW_NOISE, 1, CW_TRI, 60, 64, 64, 8, 0, 127, 0, 102, KS_OFF, 0, 0, 10),
        KS(100, CW_NOISE, 0, CW_NOISE, 64, 64, 64, 11, 3, 85, 4, 110, KS_OFF, 0, 0, 30),
        KS(96, CW_PULSE, 1, CW_NOISE, 64, 30, 64, 10, 2, 105, 10, 70, KS_OFF, 0, 0, 16),
        KS(112, CW_NOISE, 0, CW_NOISE, 64, 64, 64, 3, 3, 110, 0, 120, KS_OFF, 0, 0, 0),
        KS(64, CW_OFF, 0, CW_TRI, 62, 64, 64, 6, 0, 127, 0, 100, KS_OFF, 0, 0, 0),
        KS(79, CW_PULSE, 0, CW_PULSE, 64, 30, 70, 7, 2, 95, 6, 86, KS_OFF, 0, 16, 0),
    }};
#undef KS
static kgd_snd_t kgd_kit[KGD_NK][DRUM_LANES];
static uint8_t kgd_ready;
static void kgd_defaults(void)
{
    memcpy(kgd_kit, KGD_DEF, sizeof kgd_kit);
    kgd_ready = 1;
}
static kgd_snd_t *kgd_snd(uint32_t m, uint32_t lane)
{
    if (!kgd_ready)
        kgd_defaults();
    return &kgd_kit[m % KGD_NK][lane % DRUM_LANES];
}
static void kgd_fix(kgd_snd_t *s, uint32_t m)      /* into the ranges (values from flash) */
{
    uint32_t k;
    for (k = 0; k < KGD_NP; k++)
        s->p[k] = (uint8_t)clamp(s->p[k], KGD_MIN[m % KGD_NK][k], KGD_MAX[m % KGD_NK][k]);
    s->sat &= 3u;
    s->drv &= 127u, s->dly &= 127u, s->rev &= 127u;
}

/* a voice (drums.c: the drum voices' dsv_t memory) */
typedef struct {
    const kgd_snd_t *d;
    uint8_t m, lane, wave, frame;
    uint32_t ph, ph2, inc, inc_to, inc2;           /* carrier (C64: the oscillator), modulator; increments */
    int32_t base16, pe, kpe;                       /* pitch (1/16 semitones), its drop's envelope Q15, its factor */
    int32_t amp, amp_to, ka, ix, ix_to, ki, nz, nz_to, kn;   /* tone, index, noise: now, at the block's end, factors */
    int32_t fbp, lp, rnd;                          /* FM feedback; the noise's colour; the noise */
    int32_t gain, lg, ir;                          /* velocity Q15; LEVEL Q10; the index's scale */
    uint32_t fleft, lfsr, pw, prevph;              /* C64: samples to the next frame, the SID noise, the pulse width */
    int32_t f1, f2;
    tsvf_t c;
    int32_t fk;
} kgb_t;

static void kgd_c64_frame(kgb_t *v)                /* a new 50 Hz frame: the wave, the pitch, the pulse width */
{
    const kgd_snd_t *d = v->d;
    int32_t body = (int32_t)v->frame - (int32_t)d->p[C6_FR], p16;
    v->wave = (uint8_t)(body < 0 ? d->p[C6_W1] : d->p[C6_W2]);
    p16 = v->base16 + (body > 0 ? body * ((int32_t)d->p[C6_SWP] - 64) * 8 : 0);   /* (SWEEP: half semitones a frame) */
    v->inc = v->inc_to = ds_inc(clamp(p16, 0, 127 * 16));
    if (body > 0)
        v->pw = (v->pw + (uint32_t)(((int32_t)d->p[C6_PWM] - 64) * 16)) & 4095u;
    v->frame = (uint8_t)(v->frame < 255u ? v->frame + 1u : 255u);
    v->fleft = FS / 50u;
}

/* a hit: model m, the lane's sound, velocity, TUNE (1/16 semitones) */
static void kgd_on(kgb_t *v, uint32_t m, uint32_t lane, uint32_t vel, int32_t tune16)
{
    const kgd_snd_t *d = kgd_snd(m, lane);
    memset(v, 0, sizeof *v);
    v->d = d;
    v->m = (uint8_t)m;
    v->lane = (uint8_t)lane;
    v->gain = (int32_t)vel * 258;
    v->lg = d->p[m == KGD_FM ? FM_LVL : C6_LVL] * 1024 / 100;
    v->base16 = (int32_t)d->p[0] * 16 + tune16;
    v->rnd = (int32_t)(0x2545F491u ^ (lane * 2654435761u) ^ rng());
    v->amp = v->amp_to = 32767;
    if (m == KGD_FM) {
        v->pe = d->p[FM_BEND] ? 32767 : 0;
        v->kpe = (int32_t)ds_k32(d->p[FM_BT]);
        v->inc = v->inc_to = ds_inc(v->base16 + ((v->pe * (int32_t)d->p[FM_BEND] * 16) >> 15));
        v->ka = (int32_t)ds_k32(d->p[FM_DEC]);
        v->ix = v->ix_to = 32767;
        v->ki = (int32_t)ds_k32(d->p[FM_IDEC]);
        v->ir = (int32_t)d->p[FM_IDX] * 3259 / 127 * (64 + (int32_t)vel / 2) / 128;   /* (harder: brighter) */
        v->nz = v->nz_to = d->p[FM_NZ] * 258;
        v->kn = (int32_t)ds_k32(d->p[FM_NDEC]);
    } else {
        v->ka = (int32_t)ds_k32(KGD_SIDK[d->p[C6_DEC] & 15u]);
        v->pw = (uint32_t)d->p[C6_PW] * 32u;
        v->lfsr = 0x7FFFF8u;
        v->fk = 8192 - (int32_t)d->p[C6_RES] * 8 * 7600 / 127;
        tsvf_coef(&v->c, (int32_t)d->p[C6_CUT] << 8, (int32_t)d->p[C6_RES] * 8);
        kgd_c64_frame(v);
    }
}

static int32_t kgd_sat(int32_t x, uint32_t type, uint32_t drv)
{
    int32_t y, L = 16000;
    if (type == KS_OFF)
        return x;
    y = clamp((x * (16 + (int32_t)drv)) >> 4, -(1 << 22), 1 << 22);   /* x1 .. x9 */
    switch (type) {
    case KS_SOFT:
        return softclip(y);
    case KS_HARD:
        return clamp(y, -L, L) * 3 / 2;
    default: {                                     /* FOLD: reflected back each time it reaches +-L */
        int32_t u = (y + L) % (4 * L);
        if (u < 0) u += 4 * L;
        return (u < 2 * L ? u - L : 3 * L - u) * 3 / 2;
    }
    }
}

/* n (<= CTL) samples into out[] (as ds_render: Q15-ish); 0 when the sound has ended */
static int kgd_render(kgb_t *v, int32_t *out, uint32_t n)
{
    const kgd_snd_t *d = v->d;
    int32_t a0 = v->amp, ra = v->amp_to - v->amp, x0 = v->ix, rx = v->ix_to - v->ix, z0 = v->nz, rz = v->nz_to - v->nz;
    int32_t di = (int32_t)(v->inc_to - v->inc) >> CTL_LOG2, inc = (int32_t)v->inc;
    uint32_t i, sat = d->sat, drv = d->drv;
    if (v->m == KGD_FM) {
        uint32_t ratio = KGD_RATIO[d->p[FM_RAT] & 15u], fb = (uint32_t)d->p[FM_FB] * 480u / 127u, ncol = d->p[FM_NCOL];
        int32_t ac = ncol < 64u ? (int32_t)(ncol + 2u) * 500 : (int32_t)(129u - ncol) * 500;
        for (i = 0; i < n; i++) {
            int32_t a = a0 + ((ra * (int32_t)(i + 1u)) >> CTL_LOG2), ix = x0 + ((rx * (int32_t)(i + 1u)) >> CTL_LOG2);
            int32_t z = z0 + ((rz * (int32_t)(i + 1u)) >> CTL_LOG2), mod, car, w, x;
            mod = sine_i(v->ph2 + ((uint32_t)(v->fbp * (int32_t)fb) << 6));
            v->fbp = (mod + v->fbp) >> 1;
            car = sine_i(v->ph + ((uint32_t)(mod * mulq15(v->ir, ix)) << 6));
            v->ph += (uint32_t)inc;
            v->ph2 += ((uint32_t)inc >> 8) * ratio;
            x = mulq15(car, a);
            if (z) {
                w = (int32_t)noise32(&v->rnd) >> 17;
                v->lp += mulq15(w - v->lp, ac);
                x += mulq15(ncol < 64u ? v->lp * 2 : ncol == 64u ? w : w - v->lp, z);
            }
            x = kgd_sat(x, sat, drv);
            out[i] = ((mulq15(x, v->gain) >> 2) * v->lg) >> 8;
            inc += di;
        }
        /* the block's control: the envelopes' targets at its end */
        v->amp = v->amp_to, v->ix = v->ix_to, v->nz = v->nz_to, v->inc = v->inc_to;
        v->amp_to = (int32_t)(((uint32_t)v->amp * (uint32_t)v->ka) >> 16);
        v->ix_to = (int32_t)(((uint32_t)v->ix * (uint32_t)v->ki) >> 16);
        v->nz_to = (int32_t)(((uint32_t)v->nz * (uint32_t)v->kn) >> 16);
        v->pe = (int32_t)(((uint32_t)v->pe * (uint32_t)v->kpe) >> 16);
        v->inc_to = ds_inc(v->base16 + ((v->pe * (int32_t)d->p[FM_BEND] * 16) >> 15));
        return v->amp > 8 || v->amp_to > 8 || v->nz > 8 || v->nz_to > 8;
    }
    {   /* C64 */
        uint32_t flt = d->p[C6_FLT] & 3u;
        for (i = 0; i < n; i++) {
            int32_t a = a0 + ((ra * (int32_t)(i + 1u)) >> CTL_LOG2), w;
            uint32_t ph = v->ph;
            switch (v->wave) {
            case CW_NOISE:
                if ((ph ^ v->prevph) & ph & 0x08000000u) {   /* (the SID clocks its noise on the accumulator's bit 19) */
                    uint32_t b = ((v->lfsr >> 22) ^ (v->lfsr >> 17)) & 1u;
                    v->lfsr = ((v->lfsr << 1) | b) & 0x7FFFFFu;
                }
                w = (int32_t)(((v->lfsr >> 22 & 1u) << 7 | (v->lfsr >> 20 & 1u) << 6 | (v->lfsr >> 16 & 1u) << 5 |
                               (v->lfsr >> 13 & 1u) << 4 | (v->lfsr >> 11 & 1u) << 3 | (v->lfsr >> 7 & 1u) << 2 |
                               (v->lfsr >> 4 & 1u) << 1 | (v->lfsr >> 2 & 1u)) << 4) - 2048;
                break;
            case CW_PULSE:
                w = (ph >> 20) >= v->pw ? 2047 : -2048;
                break;
            case CW_SAW:
                w = (int32_t)(ph >> 20) - 2048;
                break;
            case CW_TRI: {
                int32_t t = (int32_t)(ph >> 19);
                w = t < 4096 ? t * 1 - 2048 : 6143 - t;
                break;
            }
            default:
                w = 0;
                break;
            }
            v->prevph = ph;
            v->ph += v->inc;
            w <<= 3;                                    /* (12 bits: +-16384) */
            if (flt) {                                  /* the SID's filter: the SVF's LP, BP or HP */
                int32_t v3 = w - v->f2, v1 = (v->c.a1 * v->f1 + v->c.a2 * v3) >> 13, v2 = v->f2 + ((v->c.a2 * v->f1 + v->c.a3 * v3) >> 13);
                v->f1 = clamp(2 * v1 - v->f1, -150000, 150000);
                v->f2 = clamp(2 * v2 - v->f2, -150000, 150000);
                w = flt == 1u ? v2 : flt == 2u ? v1 : w - ((v->fk * v1) >> 12) - v2;
            }
            w = kgd_sat(mulq15(w, a) * 2, sat, drv);
            out[i] = ((mulq15(w, v->gain) >> 2) * v->lg) >> 8;
            if (--v->fleft == 0u)
                kgd_c64_frame(v);
        }
        (void)di, (void)inc, (void)x0, (void)rx, (void)z0, (void)rz;
        v->amp = v->amp_to;
        v->amp_to = (int32_t)(((uint32_t)v->amp * (uint32_t)v->ka) >> 16);
        return v->amp > 8 || v->amp_to > 8;
    }
}
