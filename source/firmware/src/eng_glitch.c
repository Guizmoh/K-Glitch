/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH: GLITCH, digital accidents as an instrument (a synth track's engine).
 *
 * MODE:
 *   BYTE  bytebeat: one of eight formulas of a counter (SHAPE picks it), run at the key's speed
 *   BITS  an 8-bit ramp with its bits flipped by a mask (SHAPE): from a square to broken chip tones
 *   BLIP  a sine whose pitch leaps up and falls back on every retrigger (SHAPE: how far)
 *   ZAP   a laser: from SHAPE x 3 semitones above, down to the key, fast
 *   DATA  random values held a period each, the sequence starting again every SHAPE values: looped digital noise
 *   BEND  a saw whose phase jumps at random (SHAPE: how often): a circuit-bent oscillator
 * RATE retriggers inside the note (2..60 Hz, 0 none): each retrigger restarts DECAY's envelope (0: none) and, with
 * JUMP, moves the pitch at random (up to two octaves) from a sequence SEED starts each note (the same notes, the same
 * glitches). CRSH: fewer bits and held samples. TONE: a one-pole low-pass (127 open).
 * State: ph[0] the oscillator, ph[1] samples to the next retrigger, ph[2] the bytebeat counter (Q20) or the DATA step;
 * s[0] the random generator, s[1] the envelope (Q15), s[2] the DC blocker (Q6), s[3] the low-pass, s[4] the held
 * sample, s[5] its count, s[6] the jump (1/16 semitone), s[7] DATA's value. */
enum { GL_BYTE, GL_BITS, GL_BLIP, GL_ZAP, GL_DATA, GL_BEND };
static const char *const N_GLITCH_MODE[] = {"BYTE", "BITS", "BLIP", "ZAP", "DATA", "BEND"};

static uint32_t gl_inc(int32_t p16)               /* the phase increment of pitch p16 (1/16 semitone) */
{
    return pitch_inc((uint32_t)clamp(p16, 0, 2047));
}
static void glitch_note_on(track_t *t, voice_t *v)
{
    v->ph[0] = v->ph[1] = v->ph[2] = 0;
    v->s[0] = (int32_t)(((uint32_t)t->p[P_E6] + 1u) * 2654435761u | 1u);   /* SEED: the same glitches each note */
    v->s[1] = 32767;
    v->s[2] = 0;
    v->s[3] = v->s[4] = v->s[5] = v->s[6] = v->s[7] = 0;
}
/* UNISON (voice.c unison_spread): each voice its own time: the oscillator, the bytebeat counter and the retrigger
 * moved, the glitches' sequence reseeded (in phase, eight digital voices stack: +9 dB over POLY) */
static void glitch_spread(track_t *t, voice_t *v, uint32_t r)
{
    (void)t;
    v->ph[0] += r;
    v->ph[2] += (r >> 24) << 12;                    /* (bytebeat: up to 255 counts on) */
    v->ph[1] = (r >> 9) & 2047u;
    v->s[0] = (int32_t)(((uint32_t)v->s[0] ^ (r * 2654435761u)) | 1u);
}
static void glitch_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t mode = (uint32_t)p[P_E0] % 6u, shape = (uint32_t)p[P_E1], rate = (uint32_t)p[P_E2], i;
    uint32_t crsh = (uint32_t)p[P_E3], jump = (uint32_t)p[P_E4], decay = (uint32_t)p[P_E5], tone = (uint32_t)p[P_E7];
    uint32_t ph = v->ph[0], left = v->ph[1], tt = v->ph[2], inc;
    int32_t rng = v->s[0], env = v->s[1], dc = v->s[2], lp = v->s[3], held = v->s[4], hn = v->s[5], jmp = v->s[6], dv = v->s[7];
    uint32_t period = rate ? (uint32_t)FS / (2u + rate * rate * 58u / 16129u) : 0u;
    int32_t dshift = decay ? 13 - (int32_t)decay / 12 : 0;   /* the envelope: 1/2^13 .. 1/2^3 a sample */
    int32_t coef = 1200 + (int32_t)tone * 31500 / 127;       /* TONE: the low-pass's share a sample, Q15 */
    /* UNISON: BYTE and DATA have no phase to spread (a counter, a sequence): their voices stay near in phase and stack;
     * each at half its level (as the others' random phases make the sum) */
    int32_t uni_sh = p[P_VOICE] == V_UNISON && (mode == GL_BYTE || mode == GL_DATA) ? 1 : 0;
    inc = gl_inc(m->pitch16 + jmp);
    for (i = 0; i < n; i++) {
        int32_t y;
        if (period && !left--) {                   /* a retrigger */
            left = period;
            env = 32767;
            if (jump) {
                uint32_t r = noise32(&rng);
                jmp = (int32_t)(r % (jump * 24u / 127u * 2u + 1u)) - (int32_t)(jump * 24u / 127u);
                jmp *= 16;
            }
            inc = gl_inc(m->pitch16 + jmp);
            if (mode == GL_BYTE) tt = 0;
        }
        if ((i & 15u) == 0u && (mode == GL_ZAP || mode == GL_BLIP))   /* (the pitch sweeps: every 16 samples) */
            inc = gl_inc(m->pitch16 + jmp + (mode == GL_ZAP ? (env >> 8) * (int32_t)shape * 3 * 16 / 127 / 128
                                                          : ((env >> 8) * (int32_t)shape * 12 * 16 / 127 / 128)));
        switch (mode) {
        case GL_BYTE: {
            uint32_t tq, b;
            tt += inc >> 15;                       /* (Q12: about 8000 counts a second at C4, as bytebeat's) */
            tq = tt >> 12;
            switch (shape >> 4) {
            case 0: b = tq * ((tq >> 5) | (tq >> 8)); break;
            case 1: b = tq * (((tq >> 9) | (tq >> 13)) & 25u & (tq >> 6)); break;
            case 2: b = (((tq >> 6) | tq | (tq >> ((tq >> 16) & 7u))) * 10u) + ((tq >> 11) & 7u); break;
            case 3: b = tq * ((tq >> 11) & (tq >> 8) & 123u & (tq >> 3)); break;
            case 4: b = ((tq * 5u) & (tq >> 7)) | ((tq * 3u) & (tq >> 10)); break;
            case 5: b = tq * (((tq >> 12) | (tq >> 8)) & (63u & (tq >> 4))); break;
            case 6: b = (((tq >> 7) | tq | (tq >> 6)) * 10u) + 4u * ((tq & (tq >> 13)) | (tq >> 6)); break;
            default: b = ((tq * 9u) & (tq >> 4)) | ((tq * 5u) & (tq >> 7)) | ((tq * 3u) & (tq >> 10)); break;
            }
            y = ((int32_t)(b & 255u) - 128) << 7;
            break;
        }
        case GL_BITS: {
            uint32_t b = (ph >> 24) ^ (shape * 2u);
            b ^= b >> (1u + (shape >> 5));
            y = ((int32_t)(b & 255u) - 128) << 7;
            ph += inc;
            break;
        }
        case GL_BLIP:
        case GL_ZAP:
            y = sine_i(ph) >> 1;
            ph += inc;
            break;
        case GL_DATA: {
            uint32_t o = ph;
            ph += inc;
            if (ph < o) {                          /* a period: the next value; every SHAPE values the same again */
                if (++tt >= 1u + shape / 2u) {
                    tt = 0;
                    rng = (int32_t)(((uint32_t)p[P_E6] + 1u) * 2654435761u | 1u);
                }
                dv = (int32_t)(noise32(&rng) >> 17) - 16384;
            }
            y = dv;
            break;
        }
        default: {                                 /* BEND: the saw, its phase thrown at random */
            uint32_t o = ph;
            ph += inc;
            if (ph < o && (noise32(&rng) & 127u) < shape)
                ph += noise32(&rng);
            y = (int32_t)(ph >> 17) - 16384;
            break;
        }
        }
        if (dshift) {                              /* DECAY: from each retrigger */
            y = (y * env) >> 15;
            env -= env >> dshift;
        }
        if (crsh) {                                /* CRSH: held samples, fewer bits */
            if (--hn <= 0) {
                hn = 1 + (int32_t)crsh / 10;
                held = y >> (crsh / 12u) << (crsh / 12u);
            }
            y = held;
        }
        lp += ((y - lp) * coef) >> 15;
        dc += ((lp << 6) - dc) >> 11;              /* (a DC blocker, ~3 Hz: bytebeat and BITS lean to one side) */
        out[i] += voice_amp((lp - (dc >> 6)) >> uni_sh, m, i);
    }
    v->ph[0] = ph, v->ph[1] = left, v->ph[2] = tt;
    v->s[0] = rng, v->s[1] = env, v->s[2] = dc, v->s[3] = lp, v->s[4] = held, v->s[5] = hn, v->s[6] = jmp, v->s[7] = dv;
}
static const preset_t GLITCH_PRESETS[] = {
    {"BYTEBEAT", {GL_BYTE, 0, 0, 0, 0, 0, 0, 120}, {0, 60, 127, 30}, 0, 0, FX(0, 0, 10, 20)},
    {"BYTE CRUSH", {GL_BYTE, 48, 0, 70, 0, 0, 0, 95}, {0, 60, 127, 25}, 0, 0, FX(0, 0, 0, 15)},
    {"BIT SNAP", {GL_BITS, 60, 0, 40, 0, 0, 0, 110}, {0, 40, 90, 20}, 0, 0, FX(0, 0, 10, 15)},
    {"BLIP RAIN", {GL_BLIP, 90, 70, 0, 60, 60, 11, 120}, {0, 90, 100, 40}, 0, 0, FX(0, 0, 30, 40)},
    {"ZAP GUN", {GL_ZAP, 100, 30, 20, 0, 50, 0, 127}, {0, 70, 60, 30}, 0, 0, FX(0, 0, 20, 30)},
    {"DATA DUST", {GL_DATA, 20, 0, 50, 0, 0, 42, 100}, {0, 50, 110, 20}, 0, 0, FX(0, 0, 0, 20)},
    {"DATA LOOP", {GL_DATA, 8, 40, 30, 30, 40, 7, 110}, {0, 60, 100, 30}, 0, 0, FX(0, 0, 20, 25)},
    {"BENT SAW", {GL_BEND, 50, 0, 30, 0, 0, 0, 100}, {0, 60, 120, 30}, 0, 0, FX(0, 10, 0, 20)},
    {"ROBOT", {GL_BITS, 100, 90, 60, 80, 70, 3, 90}, {0, 60, 110, 30}, 0, 1, FX(0, 0, 10, 20)},
    {"CRASH BUG", {GL_BEND, 110, 100, 90, 110, 30, 99, 127}, {0, 80, 90, 40}, 0, 0, FX(10, 0, 30, 40)},
};
static const engine_t ENG_GLITCH = {
    .name = "GLITCH",
    .page_title = {"GLITCH", "MANGLE"},
    .edit = {
        {"MODE", F_ENUM, 0, 5, 0, N_GLITCH_MODE, 0},
        {"SHAPE", F_PCT, 0, 127, 0, 0, 0},
        {"RATE", F_PCT, 0, 127, 0, 0, 0},
        {"CRSH", F_PCT, 0, 127, 0, 0, 0},
        {"JUMP", F_PCT, 0, 127, 0, 0, 0},
        {"DECAY", F_PCT, 0, 127, 0, 0, 0},
        {"SEED", F_INT, 0, 127, 0, 0, 0},
        {"TONE", F_PCT, 0, 127, 120, 0, 0},
    },
    .presets = GLITCH_PRESETS,
    .npresets = NELEM(GLITCH_PRESETS),
    .note_on = glitch_note_on,
    .render = glitch_render,
    .fil_page = -1,
    .color = 0xF9ACu,                             /* (K-GLITCH's hot pink) */
    .macro = {P_E1, P_E2, P_E3, P_E7},
};
