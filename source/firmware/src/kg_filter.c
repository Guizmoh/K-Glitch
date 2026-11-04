/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH FILTER: a creative filter on the whole mix (fx.c mix_block: after the DRUM DESTROYER bank, before the LOOP's
 * recording: what it records is filtered). FX > FILTER: TYPE, FREQ (a note: the tonal types are in tune), RESO, MOVE
 * (an LFO on FREQ, up to +-3 octaves, its speed MOVEMENT's, on the transport). Its white and black keys tune FREQ to
 * their note. Included by kglitch.c; the audio interrupt.
 *   LP HP BP NOTCH  a resonant state-variable filter (A. Simper's, dsp.c), its four outputs
 *   DRIVE           saturated, then a resonant low-pass: acid
 *   COMB+ / COMB-   a feedback comb tuned to FREQ (RESO the feedback): a pitched ring / a hollow, nasal one
 *   METAL           four combs at inharmonic ratios, stereo: bells, gongs, metal sheets
 *   VOWEL           two formants morphing A E I O U as FREQ goes up: a talking filter
 *   PHASER          four all-pass stages, RESO the feedback: notches that sweep (with MOVE)
 *   RESONATOR       three very sharp band-passes at the note, its octave and its fifth: the mix rings in a chord
 * Each type change fades out and in (64 samples); the output is clamped. Memory: 4 KB for the combs. */

enum { KF_OFF, KF_LP, KF_HP, KF_BP, KF_NOTCH, KF_DRIVE, KF_COMBP, KF_COMBN, KF_METAL, KF_VOWEL, KF_PHASER, KF_RESON, KF_N };
#define KF_BUF 2048u
static int16_t kf_buf[KF_BUF];                    /* COMB: left 0..1023, right 1024..2047; METAL: four of 512 */
static struct {
    int8_t cur;                                    /* the type playing (fading to TYPE) */
    int32_t g;                                     /* its wet gain 0..32767 */
    int32_t s[2][6][2];                            /* per channel: up to six filters' two states (SVF) */
    int32_t ap[2][4][2];                           /* PHASER: per channel, four stages: x[n-1], y[n-1] */
    int32_t fbk[2];                                /* PHASER: the feedback */
    uint32_t w;                                    /* the comb buffer's write position */
} kf = {.cur = KF_OFF};
static volatile uint32_t kg_lfo_ph;               /* kg_route: the LFO's phase on the transport (MOVEMENT's speed) */

typedef struct { int32_t a1, a2, a3, k; } kf_c;
static void kf_coef(kf_c *c, int32_t note8, int32_t reso)   /* note8: a MIDI note << 8; reso 0..127 */
{
    tsvf_t t;
    int32_t cut = ((note8 - 5734) * 298) >> 8;    /* (dsp.c's cutoff index: 30 Hz at 0, 0.86 semitone each) */
    tsvf_coef(&t, cut, reso);
    c->a1 = t.a1, c->a2 = t.a2, c->a3 = t.a3;
    c->k = 8192 - reso * 7600 / 127;              /* (as tsvf_coef: the damping, Q12) */
}
static int32_t kf_svf(const kf_c *c, int32_t in, int32_t *st, int32_t *bp)   /* the low-pass; *bp the band-pass */
{
    int32_t v3 = in - st[1];
    int32_t v1 = (c->a1 * st[0] + c->a2 * v3) >> 13;
    int32_t v2 = st[1] + ((c->a2 * st[0] + c->a3 * v3) >> 13);
    st[0] = clamp(2 * v1 - st[0], -150000, 150000);
    st[1] = clamp(2 * v2 - st[1], -150000, 150000);
    *bp = v1;
    return v2;
}
static uint32_t kf_period(int32_t note8)           /* samples a period of note8 (MIDI << 8) */
{
    uint32_t inc = pitch_inc((uint32_t)clamp(note8 >> 4, 0, 2047));
    return inc ? 0xFFFFFFFFu / inc : 1023u;
}

static void kf_process(int32_t *l, int32_t *r, uint32_t n)
{
    static const int16_t VF1[5] = {78 * 256 + 26, 72 * 256 + 154, 61 * 256 + 0, 73 * 256 + 230, 62 * 256 + 205};   /* A E I O U */
    static const int16_t VF2[5] = {85 * 256 + 26, 93 * 256 + 179, 97 * 256 + 128, 79 * 256 + 128, 80 * 256 + 26};
    static const uint16_t MR[4] = {32768, 23921, 19005, 13435};   /* METAL: the combs' lengths, Q15 (inharmonic) */
    int32_t want = kf_amode & 15u ? (int32_t)(kf_amode & 15u) - 1 : song.g[G_KFTYPE];   /* (the automation, else the knobs) */
    int32_t res = kf_amode ? (int32_t)(kf_amode >> 4) * 8 + 4 : song.g[G_KFRES], note8, ch;
    uint32_t i;
    kf_c c, c2, c3;
    if (kf.cur == KF_OFF && want == KF_OFF)
        return;
    if (kf.cur == KF_OFF) {                        /* a new type: from silence states */
        kf.cur = (int8_t)want;
        kf.g = 0;
        memset(kf.s, 0, sizeof kf.s);
        memset(kf.ap, 0, sizeof kf.ap);
        kf.fbk[0] = kf.fbk[1] = 0;
        memset(kf_buf, 0, sizeof kf_buf);
    }
    note8 = (int32_t)(kf_anote ? kf_anote : (uint32_t)song.g[G_KFCUT]) << 8;
    if (song.g[G_KFMOD])                           /* MOVE: +-3 octaves at 100 % */
        note8 += sine_i(kg_lfo_ph) * (int32_t)song.g[G_KFMOD] * 36 / 100 / 128;
    note8 = clamp(note8, 12 << 8, 124 << 8);
    switch (kf.cur) {
    case KF_DRIVE:
        kf_coef(&c, note8, 40 + res * 80 / 127);
        break;
    case KF_VOWEL: {
        int32_t p = (note8 >> 8) * 4 * 256 / 127, v = p >> 8, f = p & 255;
        if (v > 3) v = 3, f = 255;
        kf_coef(&c, VF1[v] + (((VF1[v + 1] - VF1[v]) * f) >> 8), 70 + res * 50 / 127);
        kf_coef(&c2, VF2[v] + (((VF2[v + 1] - VF2[v]) * f) >> 8), 70 + res * 50 / 127);
        break;
    }
    case KF_RESON:
        kf_coef(&c, note8, 118 + res * 9 / 127);
        kf_coef(&c2, note8 + (12 << 8), 118 + res * 9 / 127);
        kf_coef(&c3, note8 + (19 << 8), 118 + res * 9 / 127);
        break;
    default:
        kf_coef(&c, note8, res);
        break;
    }
    /* (once a block, not a sample: the FM-1 divides slowly) the combs' delays, their feedback, the phaser's coefficient */
    uint32_t cd = 8u, md[4] = {6u, 6u, 6u, 6u};
    int32_t cfb = 0, cgain = 32768, mfb = 0, pha = 0;
    if (kf.cur == KF_COMBP || kf.cur == KF_COMBN) {
        cd = kf_period(note8);
        while (cd > 1023u) cd >>= 1;                  /* (below 43 Hz: octaves up, still in tune) */
        if (cd < 8u) cd = 8u;
        cfb = (12000 + res * 19000 / 127) * (kf.cur == KF_COMBN ? -1 : 1);
        cgain = 32768 - (cfb < 0 ? -cfb : cfb) / 2;
    } else if (kf.cur == KF_METAL) {
        uint32_t pp = kf_period(note8), k;
        for (k = 0; k < 4u; k++) {
            uint32_t d = (pp * MR[k]) >> 15;
            while (d > 511u) d >>= 1;                 /* (octaves up: in tune) */
            md[k] = d < 6u ? 6u : d;
        }
        mfb = 20000 + res * 11000 / 127;
    } else if (kf.cur == KF_PHASER) {
        pha = clamp(30000 - ((note8 - (24 << 8)) * 27000 / (100 << 8)), 1500, 31000);
    }
    for (i = 0; i < n; i++) {
        int32_t x[2] = {l[i], r[i]}, y[2], target = want == kf.cur ? 32767 : 0;
        if (kf.g < target) kf.g = kf.g + 512 > target ? target : kf.g + 512;
        else if (kf.g > target) kf.g = kf.g - 512 < target ? target : kf.g - 512;
        for (ch = 0; ch < 2; ch++) {
            int32_t in = x[ch], bp, lp, o = in;
            switch (kf.cur) {
            case KF_LP: o = kf_svf(&c, in, kf.s[ch][0], &bp); break;
            case KF_HP: lp = kf_svf(&c, in, kf.s[ch][0], &bp); o = in - ((c.k * bp) >> 12) - lp; break;
            case KF_BP: kf_svf(&c, in, kf.s[ch][0], &bp); o = (bp * 3) >> 1; break;
            case KF_NOTCH: kf_svf(&c, in, kf.s[ch][0], &bp); o = in - ((c.k * bp) >> 12); break;
            case KF_DRIVE: o = kf_svf(&c, softclip(in * 3) >> 1, kf.s[ch][0], &bp); break;
            case KF_COMBP:
            case KF_COMBN: {
                uint32_t base = (uint32_t)ch * 1024u;
                int32_t z = kf_buf[base + ((kf.w - cd) & 1023u)] * 2;
                o = in + ((z * cfb) >> 15);
                kf_buf[base + (kf.w & 1023u)] = (int16_t)clamp(o >> 1, -32768, 32767);
                o = ((o >> 2) * cgain) >> 13;
                break;
            }
            case KF_METAL: {
                uint32_t k;
                int32_t z[4];
                if (ch) break;                     /* (both channels at once, on the left's turn) */
                for (k = 0; k < 4u; k++) {
                    uint32_t b = k * 512u;
                    z[k] = ((x[0] + x[1]) >> 2) + ((kf_buf[b + ((kf.w - md[k]) & 511u)] * 2 * mfb) >> 15);
                    kf_buf[b + (kf.w & 511u)] = (int16_t)clamp(z[k] >> 1, -32768, 32767);
                }
                y[0] = (x[0] * 6 + (z[0] + z[2] - z[1]) * 4) >> 4;   /* (each side its own mix of the four: wide) */
                y[1] = (x[1] * 6 + (z[1] + z[3] - z[2]) * 4) >> 4;
                break;
            }
            case KF_VOWEL: {
                int32_t b2;
                kf_svf(&c, in, kf.s[ch][0], &bp);
                kf_svf(&c2, in, kf.s[ch][1], &b2);
                o = (bp * 2 + ((b2 * 3) >> 1));
                break;
            }
            case KF_PHASER: {
                int32_t a = pha, v = in + ((kf.fbk[ch] * res) >> 8), k;
                for (k = 0; k < 4; k++) {          /* y = a (x - y[n-1]) + x[n-1] */
                    int32_t yy = ((a * (v - kf.ap[ch][k][1])) >> 15) + kf.ap[ch][k][0];
                    kf.ap[ch][k][0] = v;
                    kf.ap[ch][k][1] = clamp(yy, -150000, 150000);
                    v = kf.ap[ch][k][1];
                }
                kf.fbk[ch] = v >> 1;
                o = (in + v) >> 1;
                break;
            }
            case KF_RESON: {
                int32_t b2, b3;
                kf_svf(&c, in >> 1, kf.s[ch][0], &bp);
                kf_svf(&c2, in >> 1, kf.s[ch][1], &b2);
                kf_svf(&c3, in >> 1, kf.s[ch][2], &b3);
                o = (in >> 2) + bp + b2 + (b3 >> 1);
                break;
            }
            default:
                break;
            }
            if (kf.cur != KF_METAL) y[ch] = o;
        }
        kf.w++;
        l[i] = clamp(x[0] + (((clamp(y[0] - x[0], -(1 << 20), 1 << 20) >> 3) * (kf.g >> 3)) >> 9), -(1 << 21), (1 << 21) - 1);
        r[i] = clamp(x[1] + (((clamp(y[1] - x[1], -(1 << 20), 1 << 20) >> 3) * (kf.g >> 3)) >> 9), -(1 << 21), (1 << 21) - 1);
    }
    if (kf.g == 0 && want != kf.cur)               /* faded out: the next type (or none) on the next block */
        kf.cur = KF_OFF;
}
