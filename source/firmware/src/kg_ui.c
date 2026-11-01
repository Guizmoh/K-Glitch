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
    if (fx >= (int32_t)PUNCH_NFX) {                   /* the DRUM DESTROYER bank */
        static const uint8_t F[DD_N] = {0, 0, 3, 2, 2, 2, 1, 1, 0, 1, 0, 2, 3, 3, 3, 3};
        static const uint16_t C[4] = {KG_CYAN, KG_VIO, KG_AMB, KG_RED};
        return C[F[(fx - (int32_t)PUNCH_NFX) % DD_N]];
    }
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

/* the page: 0 DESTROY, 1 DRUMS, 2 CHOP, 3 SEQ, 4 BANK, 5 CHAOS, 6 SETUP */
UI_SMALL static uint32_t kg_pageno(void)
{
    static const uint8_t ID[7] = {G_KCHAOS, G_KSTYLE, G_KMUTR, G_KSTEP, G_KBANK, G_KCMODE, G_KTGT};
    uint32_t id = cur_page()->id[0], i;
    for (i = 0; i < 7u; i++)
        if (ID[i] == id) return i;
    return 0;
}

UI_SMALL static void kg_rbox(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t fill, uint16_t edge)   /* rounded */
{
    cv_rect(x + 2, y, w - 4, h, edge);
    cv_rect(x, y + 2, w, h - 4, edge);
    cv_rect(x + 1, y + 1, w - 2, h - 2, edge);
    cv_rect(x + 2, y + 1, w - 4, h - 2, fill);
    cv_rect(x + 1, y + 2, w - 2, h - 4, fill);
}
/* the drum track as a grid: 32 16ths, four rows (kicks, snares and claps, hats and cymbals, the rest); a hit drawn
 * bright (hard), plain, or dim (soft, ghost), with a notch when it rolls; orange where CHOP changed the ORIGINAL; the
 * 16th playing marked */
UI_SMALL static void kg_drumgrid(int32_t y0)
{
    static const uint16_t ROW[DRUM_LANES] = {0, 0, 1, 1, 2, 2, 2, 1, 1, 3, 3, 2, 2, 2, 3, 3};   /* a lane's row */
    static const char *const RL[4] = {"K", "S", "H", "P"};
    uint32_t s, l, r, len = (uint32_t)TDRUM->p[P_SLEN], play = song.playing ? TDRUM->seq_idx : 0xFFFFu;
    for (r = 0; r < 4u; r++) kg_t5(13, y0 + (int32_t)r * 9, RL[r], KG_MUTE);
    for (s = 0; s < 32u; s++) {
        int32_t x = 22 + (int32_t)s * 6 + (int32_t)(s / 4u);
        const dstep_t *d = &TDRUM->dstep[s];
        uint32_t lv[4] = {0, 0, 0, 0}, rat[4] = {0, 0, 0, 0}, mut[4] = {0, 0, 0, 0};
        for (l = 0; l < DRUM_LANES; l++) {
            uint32_t rr = ROW[l], has = dstep_has(d, l);
            if (has) {
                uint32_t v = dstep_lvl(d, l), w = v == LV_HARD ? 3u : v == LV_NORM ? 2u : 1u;
                if (w > lv[rr]) lv[rr] = w;
                rat[rr] |= (d->rat[l >> 2] >> ((l & 3u) * 2u)) & 3u;
            }
            if (kr.mutated && (uint32_t)has != (uint32_t)dstep_has(&kr.d[s], l)) mut[rr] = 1;
        }
        if (kr.mutated && TDRUM->micro[s] != kr.micro[s]) mut[0] = mut[1] = mut[2] = mut[3] = mut[0] | 2u;
        for (r = 0; r < 4u; r++) {
            int32_t y = y0 + (int32_t)r * 9;
            uint16_t c = mut[r] & 1u ? KG_ACC : r == 0u ? KG_RED : r == 1u ? KG_AMB : r == 2u ? KG_CYAN : KG_VIO;
            if (s >= len) { cv_rect(x, y + 3, 5, 1, KG_RULE); continue; }
            if (lv[r]) {
                cv_rect(x, y, 5, 7, kg_mix(KG_PANEL, c, lv[r] == 3u ? 256u : lv[r] == 2u ? 200u : 110u));
                if (rat[r]) cv_rect(x + 1, y + 3, 3, 1, KG_PANEL);
            } else {
                cv_rect(x + 2, y + 3, 1, 1, (s & 3u) == 0u ? KG_MUTE : KG_RULE);
                if (mut[r] & 1u) kg_box(x, y, 5, 7, KG_ACC);   /* (a hit taken out) */
            }
        }
        if (mut[0] & 2u) cv_rect(x + 1, y0 + 35, 3, 1, KG_ACC);   /* (nudged early or late) */
        if (s == play % 32u && s < len) cv_rect(x, y0 - 3, 5, 2, KG_INK);
    }
}
/* the 16 pads: the bank on the white keys, the effect playing lit, the one selected (ALGORITHM, a pad) underlined,
 * each DRUM DESTROYER pad's INTENSITY as a bar under it */
UI_SMALL static void kg_pads16(int32_t now)
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        int32_t cx = 13 + (int32_t)(i & 7u) * 27, cy = 52 + (int32_t)(i >> 3) * 22;
        int32_t fx = song.g[G_KBANK] ? (int32_t)(PUNCH_NFX + i) : (int32_t)i, lit = fx == now;
        uint16_t c = kg_fam(fx);
        cv_rect(cx, cy, 25, 16, kg_mix(KG_PANEL, c, lit ? 256u : 170u));
        kg_t5(cx + (25 - kg_t5w(KG_SHORT[fx])) / 2, cy + 5, KG_SHORT[fx], KG_BG);
        if (lit) kg_box(cx - 1, cy - 1, 27, 18, KG_INK);
        if (song.g[G_KBANK]) {
            cv_rect(cx, cy + 17, 25, 2, KG_RULE);
            cv_rect(cx, cy + 17, (int32_t)kd_int[i] / 4, 2, i == kg_sel ? KG_ACC : kg_mix(KG_RULE, KG_INK, 150));
            if (i == kg_sel) kg_box(cx - 2, cy - 2, 29, 22, KG_ACC);
        }
    }
}
UI_SMALL static void kg_paint(void)
{
    static const char *const TAB[7] = {"DESTROY", "DRUMS", "CHOP", "SEQ", "BANK", "CHAOS", "SETUP"};
    static const char *const TITLE[7] = {"DRUM DESTROYER", "DRUMS", "CHOP / MUTATE", "EFFECT SEQUENCE", "THE 16 PADS",
                                         "CHAOS", "SETUP / MPC"};
    static const char *const OUT[3] = {"FM-1", "MPC", "BOTH"};
    static const char *const HINT[7] = {"ALGO FX | PRESETS INTENS | KEYS PADS", "K4 GENERATE | THE KEYS PLAY THE DRUMS",
                                        "K1 MUTATE | K2 RESTORE | KEYS PADS", "WHITE KEY: STEP | K2 ITS FX | K4 DICE",
                                        "KEYS PADS | HOLD: THE EFFECT STAYS", "AUTO + LFO + MUTATIONS ON THE BAR",
                                        "OUT: FM-1, MPC, BOTH | CH: THE MPC'S"};
    static const char *const SRC[4] = {"", "KEY", "SEQ", "AUTO"};
    const page_t *pg = cur_page();
    uint32_t pn = kg_pageno(), i, out = (uint32_t)song.g[G_KTGT] % 3u;
    int32_t now = kg.now, play = song.playing ? (int32_t)kg.step : -1, cur = pn == 3u ? (int32_t)song.g[G_KSTEP] - 1 : -1, x;
    char s[48], k[64];
    cv_rect(0, 0, 240, 240, KG_BG);
    /* the ribbon: the seven pages as tabs, scrolled so the one shown (an orange pill) stays in sight */
    {
        int32_t pos[7], w[7], tot = 0, off;
        for (i = 0; i < 7u; i++) { w[i] = kg_t5w(TAB[i]) + 14; pos[i] = tot; tot += w[i] + 3; }
        off = pos[pn] + w[pn] / 2 - 96;
        if (off > tot - 196) off = tot - 196;
        if (off < 0) off = 0;
        for (i = 0; i < 7u; i++) {
            int32_t tx = 4 + pos[i] - off;
            if (tx < 2 || tx + w[i] > 198) continue;   /* (only whole tabs) */
            if (i == pn) kg_rbox(tx, 2, w[i], 13, KG_ACC, KG_ACC);
            kg_t5(tx + 7, 5, TAB[i], i == pn ? KG_BG : KG_MUTE);
        }
    }
    kg_u2a(s, (uint32_t)song.g[G_BPM]);
    kg_t5(224 - kg_t5w(s), 5, s, KG_INK);
    if (song.playing) for (i = 0; i < 7u; i++) cv_rect(229, 5 + (int32_t)i, (int32_t)(i < 4u ? i + 1u : 7u - i), 1, KG_ACC);
    else cv_rect(229, 6, 6, 6, KG_MUTE);
    /* the card: title, the orange line under it (or the last message), the tags */
    kg_rbox(3, 19, 234, 219, KG_CARD, kg_mix(KG_CARD, KG_ACC, 150));
    cv_text(10, 21, &FONT_S, TITLE[pn], KG_INK);
    cv_text(11, 21, &FONT_S, TITLE[pn], KG_INK);           /* (bold) */
    s[0] = 0;
    if (ui.msg_t) str_cpy(s, ui.msg, sizeof s);
    else switch (pn) {
    case 0:
        str_cpy(s, DD_NAME[kg_sel % DD_N], sizeof s);
        str_cpy(s + str_len(s), " | INTENS ", sizeof s - str_len(s));
        kg_u2a(s + str_len(s), kd_int[kg_sel % DD_N]);
        str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
        break;
    case 1:
        str_cpy(s, N_KSTYLE_P[song.g[G_KSTYLE] % KS_N], sizeof s);
        str_cpy(s + str_len(s), " | KIT ", sizeof s - str_len(s));
        str_cpy(s + str_len(s), DRUM_KIT_NAMES[drum_kit()], sizeof s - str_len(s));
        break;
    case 2:
        if (kr.mutated) {
            str_cpy(s, "MUTATED | ", sizeof s);
            kg_u2a(s + str_len(s), kr.changed);
            str_cpy(s + str_len(s), " CHANGES", sizeof s - str_len(s));
        } else
            str_cpy(s, kr.has ? "THE ORIGINAL" : "NOTHING MUTATED YET", sizeof s);
        break;
    case 3: {
        uint32_t n = 0;
        for (i = 0; i < KG_STEPS; i++) n += kg.seq[i] >= 0;
        str_cpy(s, song.g[G_KSEQ] ? "SEQ ON | " : "SEQ OFF | ", sizeof s);
        kg_u2a(s + str_len(s), n);
        str_cpy(s + str_len(s), " OF 16 STEPS", sizeof s - str_len(s));
        break;
    }
    case 4:
        str_cpy(s, song.g[G_KBANK] ? "DRUM DESTROYER | " : "SLOOP FX | ", sizeof s);
        str_cpy(s + str_len(s), DD_NAME[kg_sel % DD_N], sizeof s - str_len(s));
        break;
    case 5:
        str_cpy(s, song.g[G_KCMODE] ? "CHAOS MODE ON | " : "CHAOS MODE OFF | ", sizeof s);
        kg_u2a(s + str_len(s), (uint32_t)song.g[G_KCHAOS]);
        str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
        break;
    default:
        str_cpy(s, "OUT ", sizeof s);
        str_cpy(s + str_len(s), OUT[out], sizeof s - str_len(s));
        str_cpy(s + str_len(s), " | MPC CH ", sizeof s - str_len(s));
        if (song.g[G_KCCCH]) kg_u2a(s + str_len(s), (uint32_t)song.g[G_KCCCH]);
        else str_cpy(s + str_len(s), "OFF", sizeof s - str_len(s));
        break;
    }
    kg_t5(11, 39, s, ui.msg_t || (ui.hot_t && ui.hot_col == 4u) ? KG_INK : KG_ACC);
    {   /* the tags, right to left: OUT, then what runs on its own */
        int32_t tx = 230 - kg_t5w(OUT[out]) - 8;
        kg_tag(tx, 24, OUT[out], out == KG_OUT_FM1 ? KG_RULE : KG_ACC, out == KG_OUT_FM1 ? KG_INK : KG_BG);
        if (song.g[G_KCMODE]) { tx -= kg_t5w("CHAOS") + 12; kg_tag(tx, 24, "CHAOS", KG_RED, KG_BG); }
        else if (song.g[G_KAUTO]) { tx -= kg_t5w("AUTO") + 12; kg_tag(tx, 24, "AUTO", KG_CYAN, KG_BG); }
        if (song.g[G_KHOLD]) { tx -= kg_t5w("HOLD") + 12; kg_tag(tx, 24, "HOLD", KG_AMB, KG_BG); }
        if ((song.g[G_KLFO] || song.g[G_KCMODE]) && tx > 150) { tx -= kg_t5w("LFO") + 12; kg_tag(tx, 24, "LFO", KG_VIO, KG_BG); }
    }
    /* the graph panel: the pads, the drums or the effect sequence; the effect playing and the mix under it */
    kg_rbox(9, 49, 222, 76, KG_PANEL, KG_PANEL);
    if (pn == 1u || pn == 2u || pn == 5u)
        kg_drumgrid(55);
    else if (pn == 3u)
        for (i = 0; i < KG_STEPS; i++) {
            int32_t cx = 13 + (int32_t)(i & 7u) * 27, cy = 53 + (int32_t)(i >> 3) * 22, fx = kg.seq[i], lit = (int32_t)i == play;
            if (fx >= 0 && fx < (int32_t)KG_NFX) {
                uint16_t c = kg_fam(fx);
                cv_rect(cx, cy, 25, 18, kg_mix(KG_PANEL, c, lit ? 256u : 180u));
                kg_t5(cx + (25 - kg_t5w(KG_SHORT[fx])) / 2, cy + 6, KG_SHORT[fx], KG_BG);
            } else {
                kg_box(cx, cy, 25, 18, kg_mix(KG_PANEL, KG_RULE, 170));
                cv_rect(cx + 11, cy + 8, 2, 2, (i & 3u) == 0u ? KG_MUTE : KG_RULE);
            }
            if (lit) kg_box(cx - 1, cy - 1, 27, 20, KG_INK);
            if ((int32_t)i == cur) cv_rect(cx + 3, cy + 19, 19, 2, KG_ACC);
        }
    else
        kg_pads16(now);
    {
        int32_t ok = now >= 0 && now < (int32_t)KG_NFX;
        const char *n = ok ? kg_name(now) : song.playing ? "READY" : "STOPPED";
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
    {   /* the four knobs: K1 X | K2 Y ..., shorter when it would not fit */
        uint32_t form;
        for (form = 0; form < 3u; form++) {
            k[0] = 0;
            for (i = 0; i < 4u; i++) {
                int16_t *vp;
                const param_desc_t *d = page_desc(pg, i, &vp);
                char t[4] = {'K', (char)('1' + i), ' ', 0};
                if (!d) continue;
                if (k[0]) str_cpy(k + str_len(k), form < 2u ? " | " : " ", sizeof k - str_len(k));
                str_cpy(k + str_len(k), form ? t + 1 : t, sizeof k - str_len(k));
                str_cpy(k + str_len(k), d->label, sizeof k - str_len(k));
            }
            if (kg_t5w(k) <= 228) break;
        }
    }
    kg_t5(120 - kg_t5w(k) / 2, 216, k, KG_INK);
    kg_t5(120 - kg_t5w(HINT[pn]) / 2, 227, HINT[pn], KG_ACC);
}

/* the mix as a live line in the panel (x 110..225, y 96..119): its own small canvas, every other frame */
UI_SMALL static void kg_scope(void)
{
    uint32_t x, w = scope_w;
    int32_t py = 10;
    uint16_t c = kg.now >= 0 && kg.now < (int8_t)KG_NFX ? kg_fam(kg.now) : KG_ACC;
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
    for (i = G_KCHAOS; i < G_COUNT; i++) h = (h ^ (uint32_t)song.g[i]) * 16777619u;
    for (i = 0; i < DD_N; i++) h = (h ^ kd_int[i]) * 16777619u;
    h ^= (uint32_t)song.g[G_BPM] * 104729u + (ui.hot_t ? ui.hot_col + 1u : 0u) * 13u + kg_sel * 7u + (uint32_t)song.g[G_SWING];
    if (kg_pageno() == 1u || kg_pageno() == 2u || kg_pageno() == 5u)   /* the drum grid: the pattern, the 16th playing */
        h ^= kr_sig() + (song.playing ? TDRUM->seq_idx + 1u : 0u) * 31337u + kr.changed + (uint32_t)TDRUM->p[P_E0] * 7u;
    if (ui.msg_t) h = str_hash(h ^ 0x9E37u, ui.msg);
    return h;
}

UI_SMALL static void kg_screen_draw(void)
{
    uint32_t sig, pn = kg_pageno();
    kg_pads = (uint8_t)(pn == 3u ? 2u : pn == 1u ? 0u : 1u);   /* the keys: SEQ's steps, the drums (DRUMS), else pads */
    if (kg_keystep) {                                   /* SEQ: a white key picked the step */
        song.g[G_KSTEP] = kg_keystep;
        song.g[G_KSFX] = (int16_t)(kg.seq[(kg_keystep - 1u) & 15u] + 1);
        kg_keystep = 0;
    }
    song.g[G_KINT] = kd_int[kg_sel % DD_N];
    sig = kg_signature();
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
