/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH's own screens: the FX pages DESTROY, D-SEQ and D-SETUP drawn whole (ui_draw.c ui_draw hands them here),
 * in the K-WASUB manner.
 * Its base is K-WASUB's RIBBON CLASSIC theme:
 *   ribbon       the three pages as tabs (the one shown an orange pill), the tempo, PLAY / STOP
 *   card         a bold title, an orange status line (or the last message), OUT and AUTO as tags
 *   graph panel  the 16 steps of the effect sequence (the step playing framed white, the one edited underlined), the
 *                effect playing now and who plays it, and the mix as a live line (redrawn alone, every other frame)
 *   rows         K1..K4: name, value in cyan; the one just turned filled orange
 *   footer       an orange rule, what the four knobs are, what the buttons do
 * Families of effects: loops and stutters cyan, filters and kills violet, time and pitch amber, destruction red. */
#include "kg_font5.h"

/* the K-WASUB "ribbon classic" palette: slate, an orange accent, cyan values */
#define KG_BG RGB(20, 27, 35)
#define KG_BAR RGB(20, 27, 35)
#define KG_CARD RGB(34, 44, 56)
#define KG_PANEL RGB(24, 32, 42)
#define KG_ROW RGB(32, 42, 55)
#define KG_RULE RGB(66, 82, 100)
#define KG_INK RGB(236, 240, 244)
#define KG_MUTE RGB(138, 150, 164)
#define KG_ACC RGB(255, 136, 56)                  /* the accent: orange */
#define KG_ACCD RGB(120, 52, 18)                  /* the active row's fill */
#define KG_VAL RGB(84, 200, 240)                  /* values: cyan */
#define KG_CYAN RGB(84, 200, 240)
#define KG_VIO RGB(170, 132, 255)
#define KG_AMB RGB(255, 196, 48)
#define KG_RED RGB(255, 84, 72)

static uint8_t kg_shown;                            /* the K-GLITCH screen is up (leaving it clears the screen) */
static uint32_t kg_sig;                             /* what it last drew */

UI_SMALL static uint16_t kg_mix(uint16_t a, uint16_t b, uint32_t t)   /* a .. b, t 0..256 */
{
    uint32_t r = ((a >> 11) * (256u - t) + (b >> 11) * t) >> 8, g = (((a >> 5) & 63u) * (256u - t) + ((b >> 5) & 63u) * t) >> 8,
             bl = ((a & 31u) * (256u - t) + (b & 31u) * t) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}
UI_SMALL static uint16_t kg_fam(int32_t fx)
{
    if (fx <= PX_STUT || fx == PX_LOOP8T) return KG_CYAN;
    if ((fx >= PX_LPF && fx <= PX_TEL) || (fx >= PX_KLOW && fx <= PX_KHIGH)) return KG_VIO;
    if (fx == PX_REV || fx == PX_STOP || fx == PX_HALF || fx == PX_RWD || fx == PX_OCT || fx == PX_FREEZE) return KG_AMB;
    return KG_RED;
}
/* the 5 x 7 font: x, y the top left; returns the x after the text */
UI_SMALL static int32_t kg_t5(int32_t x, int32_t y, const char *s, uint16_t c)
{
    for (; *s; s++, x += 6) {
        uint32_t ch = (uint8_t)*s, r, k;
        if (ch >= 'a' && ch <= 'z') ch -= 32u;
        if (ch < 32u || ch > 126u) continue;
        for (r = 0; r < 7u; r++)
            for (k = 0; k < 5u; k++)
                if (KG_FONT5[ch - 32u][r] >> (4u - k) & 1u) cv_pset(x + (int32_t)k, y + (int32_t)r, c);
    }
    return x;
}
UI_SMALL static int32_t kg_t5w(const char *s) { return (int32_t)str_len(s) * 6 - 1; }
UI_SMALL static void kg_box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* a 1 px frame */
{
    cv_rect(x, y, w, 1, c);
    cv_rect(x, y + h - 1, w, 1, c);
    cv_rect(x, y, 1, h, c);
    cv_rect(x + w - 1, y, 1, h, c);
}
UI_SMALL static void kg_tag(int32_t x, int32_t y, const char *s, uint16_t bg, uint16_t fg)   /* a filled tag, 11 px */
{
    int32_t w = kg_t5w(s) + 8;
    cv_rect(x + 1, y, w - 2, 11, bg);
    cv_rect(x, y + 1, w, 9, bg);
    kg_t5(x + 4, y + 2, s, fg);
}
static void kg_u2a(char *b, uint32_t v)            /* an unsigned number, written */
{
    char t[12];
    uint32_t n = 0;
    do t[n++] = (char)('0' + v % 10u); while ((v /= 10u) && n < 11u);
    while (n) *b++ = t[--n];
    *b = 0;
}

/* the page in its family: 1 DESTROY, 2 D-SEQ, 3 D-SETUP */
UI_SMALL static uint32_t kg_pageno(void) { return cur_page()->id[0] == G_KCHAOS ? 0u : cur_page()->id[0] == G_KSTEP ? 1u : 2u; }

UI_SMALL static void kg_rbox(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t fill, uint16_t edge)   /* rounded */
{
    cv_rect(x + 2, y, w - 4, h, edge);
    cv_rect(x, y + 2, w, h - 4, edge);
    cv_rect(x + 1, y + 1, w - 2, h - 2, edge);
    cv_rect(x + 2, y + 1, w - 4, h - 2, fill);
    cv_rect(x + 1, y + 2, w - 2, h - 4, fill);
}
UI_SMALL static void kg_paint(void)
{
    static const char *const TAB[3] = {"DESTROY", "FX SEQ", "SETUP"};
    static const char *const TITLE[3] = {"DRUM DESTROYER", "EFFECT SEQUENCE", "SETUP / MPC"};
    static const char *const OUT[3] = {"FM-1", "MPC", "BOTH"};
    static const char *const HINT[3] = {"SELECT: PAGE | FX HELD: PUNCH-IN", "SELECT: PAGE | K4: ROLL THE DICE",
                                        "SELECT: PAGE | K4: LOAD A BREAK"};
    static const char *const SRC[4] = {"", "KEY", "SEQ", "AUTO"};
    const page_t *pg = cur_page();
    uint32_t pn = kg_pageno(), i, out = (uint32_t)song.g[G_KTGT] % 3u;
    int32_t now = kg.now, play = song.playing ? (int32_t)kg.step : -1, cur = pn == 1u ? (int32_t)song.g[G_KSTEP] - 1 : -1, x;
    char s[48], k[64];
    cv_rect(0, 0, 240, 240, KG_BG);
    /* the ribbon: the three pages as tabs, the one shown an orange pill; the tempo and PLAY at the right */
    for (i = 0, x = 6; i < 3u; i++) {
        int32_t w = kg_t5w(TAB[i]) + 14;
        if (i == pn) kg_rbox(x, 2, w, 13, KG_ACC, KG_ACC);
        kg_t5(x + 7, 5, TAB[i], i == pn ? KG_BG : KG_MUTE);
        x += w + 4;
    }
    kg_u2a(s, (uint32_t)song.g[G_BPM]);
    kg_t5(224 - kg_t5w(s), 5, s, KG_INK);
    if (song.playing) for (i = 0; i < 7u; i++) cv_rect(229, 5 + (int32_t)i, (int32_t)(i < 4u ? i + 1u : 7u - i), 1, KG_ACC);
    else cv_rect(229, 6, 6, 6, KG_MUTE);
    /* the card: title, the orange line under it (or the last message), OUT */
    kg_rbox(3, 19, 234, 219, KG_CARD, kg_mix(KG_CARD, KG_ACC, 150));
    cv_text(10, 21, &FONT_S, TITLE[pn], KG_INK);
    cv_text(11, 21, &FONT_S, TITLE[pn], KG_INK);           /* (bold) */
    s[0] = 0;
    if (ui.msg_t) str_cpy(s, ui.msg, sizeof s);
    else if (pn == 0u) {
        str_cpy(s, "CHAOS ", sizeof s); kg_u2a(s + str_len(s), (uint32_t)song.g[G_KCHAOS]);
        str_cpy(s + str_len(s), "% | DENS ", sizeof s - str_len(s)); kg_u2a(s + str_len(s), (uint32_t)song.g[G_KDENS]);
        str_cpy(s + str_len(s), "% | MOVE ", sizeof s - str_len(s)); kg_u2a(s + str_len(s), (uint32_t)song.g[G_KMOVE]);
        str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
    } else if (pn == 1u) {
        uint32_t n = 0;
        for (i = 0; i < KG_STEPS; i++) n += kg.seq[i] >= 0;
        str_cpy(s, song.g[G_KSEQ] ? "SEQ ON | " : "SEQ OFF | ", sizeof s);
        kg_u2a(s + str_len(s), n);
        str_cpy(s + str_len(s), " OF 16 STEPS", sizeof s - str_len(s));
    } else {
        str_cpy(s, song.g[G_KAUTO] ? "AUTO ON | MPC CH " : "AUTO OFF | MPC CH ", sizeof s);
        if (song.g[G_KCCCH]) kg_u2a(s + str_len(s), (uint32_t)song.g[G_KCCCH]);
        else str_cpy(s + str_len(s), "OFF", sizeof s - str_len(s));
    }
    kg_t5(11, 39, s, ui.msg_t ? KG_INK : KG_ACC);
    kg_tag(230 - kg_t5w(OUT[out]) - 8, 24, OUT[out], out == KG_OUT_FM1 ? KG_RULE : KG_ACC, out == KG_OUT_FM1 ? KG_INK : KG_BG);
    if (song.g[G_KAUTO]) kg_tag(230 - kg_t5w(OUT[out]) - kg_t5w("AUTO") - 20, 24, "AUTO", KG_CYAN, KG_BG);
    /* the graph panel: the 16 steps, the effect playing, the mix */
    kg_rbox(9, 49, 222, 76, KG_PANEL, KG_PANEL);
    for (i = 0; i < KG_STEPS; i++) {
        int32_t cx = 13 + (int32_t)(i & 7u) * 27, cy = 53 + (int32_t)(i >> 3) * 22, fx = kg.seq[i];
        if (fx >= 0 && fx < (int32_t)PUNCH_NFX) {
            uint16_t c = kg_fam(fx);
            cv_rect(cx, cy, 25, 18, kg_mix(KG_PANEL, c, (int32_t)i == play ? 256u : 180u));
            kg_t5(cx + (25 - kg_t5w(KG_SHORT[fx])) / 2, cy + 6, KG_SHORT[fx], KG_BG);
        } else {
            kg_box(cx, cy, 25, 18, kg_mix(KG_PANEL, KG_RULE, 170));
            cv_rect(cx + 11, cy + 8, 2, 2, (i & 3u) == 0u ? KG_MUTE : KG_RULE);
        }
        if ((int32_t)i == play) kg_box(cx - 1, cy - 1, 27, 20, KG_INK);
        if ((int32_t)i == cur) cv_rect(cx + 3, cy + 19, 19, 2, KG_ACC);
    }
    {
        int32_t ok = now >= 0 && now < (int32_t)PUNCH_NFX;
        const char *n = ok ? PUNCH_NAME[now] : song.playing ? "READY" : "STOPPED";
        x = kg_t5(14, 99, n, ok ? kg_fam(now) : KG_MUTE);
        if (ok) kg_t5(x + 6, 99, SRC[kg.src & 3u], KG_MUTE);
    }
    /* the rows: K1..K4, the one just turned orange */
    for (i = 0; i < 4u; i++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, i, &vp);
        int32_t ry = 130 + (int32_t)i * 20, hot = ui.hot_t && ui.hot_col == i;
        kg_rbox(9, ry, 222, 18, hot ? KG_ACCD : KG_ROW, hot ? KG_ACC : KG_RULE);
        if (d && vp) {
            const char *unit;
            char v[12], l[16];
            str_cpy(l, d->label, sizeof l);
            cv_text(15, ry + 1, &FONT_S, l, KG_INK);
            param_format(d, *vp, v, &unit);
            str_cpy(l, v, sizeof l);
            if (unit && unit[0]) { str_cpy(l + str_len(l), " ", sizeof l - str_len(l)); str_cpy(l + str_len(l), unit, sizeof l - str_len(l)); }
            cv_text(225 - text_w(&FONT_S, l), ry + 1, &FONT_S, l, hot ? KG_ACC : KG_VAL);
        } else {
            cv_text(15, ry + 1, &FONT_S, "--", KG_RULE);
        }
    }
    /* the footer: the orange rule, the four knobs, the buttons */
    cv_rect(4, 212, 232, 1, KG_ACC);
    k[0] = 0;
    for (i = 0; i < 4u; i++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(pg, i, &vp);
        char t[4] = {'K', (char)('1' + i), ' ', 0};
        if (!d) continue;
        if (k[0]) str_cpy(k + str_len(k), " | ", sizeof k - str_len(k));
        str_cpy(k + str_len(k), t, sizeof k - str_len(k));
        str_cpy(k + str_len(k), d->label, sizeof k - str_len(k));
    }
    kg_t5(120 - kg_t5w(k) / 2, 216, k, KG_INK);
    kg_t5(120 - kg_t5w(HINT[pn]) / 2, 227, HINT[pn], KG_ACC);
}

/* the mix as a live line in the panel (x 110..225, y 96..119): its own small canvas, every other frame */
UI_SMALL static void kg_scope(void)
{
    uint32_t x, w = scope_w;
    int32_t py = 10;
    uint16_t c = kg.now >= 0 && kg.now < (int8_t)PUNCH_NFX ? kg_fam(kg.now) : KG_ACC;
    cv_begin(116, 21, KG_PANEL);
    cv_oy = 0;
    cv_rect(0, 10, 116, 1, kg_mix(KG_PANEL, KG_RULE, 160));
    for (x = 0; x < 116u; x++) {
        int32_t v = scope_buf[(w - 464u + x * 4u) & (SCOPE_N - 1u)], y = 10 - v / 1400;
        if (y < 0) y = 0;
        if (y > 20) y = 20;
        if (x) {
            cv_line((int32_t)x - 1, py, (int32_t)x, y, c);
            cv_pset((int32_t)x, y + 1, kg_mix(KG_PANEL, c, 90));   /* (the glow under the line) */
        }
        py = y;
    }
    cv_blit(112, 99);
}

UI_SMALL static uint32_t kg_signature(void)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < KG_STEPS; i++) h = (h ^ (uint32_t)(kg.seq[i] + 2)) * 16777619u;
    h ^= (song.playing ? kg.step + 1u : 0u) * 977u + (uint32_t)(kg.now + 2) * 65537u + kg.src * 31u + ui.page * 7919u;
    for (i = G_KCHAOS; i <= G_KBRK; i++) h = (h ^ (uint32_t)song.g[i]) * 16777619u;
    h ^= (uint32_t)song.g[G_BPM] * 104729u + (ui.hot_t ? ui.hot_col + 1u : 0u) * 13u;
    if (ui.msg_t) h = str_hash(h ^ 0x9E37u, ui.msg);
    return h;
}

UI_SMALL static void kg_screen_draw(void)
{
    uint32_t sig = kg_signature();
    if (!kg_shown || ui.force || sig != kg_sig) {
        uint32_t band;
        kg_shown = 1;
        kg_sig = sig;
        for (band = 0; band < 2u; band++) {             /* the canvas holds 124 rows: two bands of 120 */
            cv_begin(240, 120, KG_BG);
            cv_oy = -(int32_t)band * 120;
            kg_paint();
            cv_oy = 0;
            cv_blit(0, band * 120u);
        }
    }
    if (ui.frame & 1u)
        kg_scope();
}
