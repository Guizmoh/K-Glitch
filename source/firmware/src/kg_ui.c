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
        static const uint8_t F[DD_N] = {0, 0, 3, 2, 2, 2, 1, 1, 0, 1, 0, 2, 3, 3, 3, 3,
                                        0, 3, 3, 0, 1, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};   /* (BEAT) */
        static const uint16_t C[4] = {KG_CYAN, KG_VIO, KG_AMB, KG_RED};
        return C[F[(fx - (int32_t)PUNCH_NFX) % DD_N]];
    }
    if (fx <= PX_STUT || fx == PX_LOOP8T) return KG_CYAN;
    if ((fx >= PX_LPF && fx <= PX_TEL) || (fx >= PX_KLOW && fx <= PX_KHIGH)) return KG_VIO;
    if (fx == PX_REV || fx == PX_STOP || fx == PX_HALF || fx == PX_RWD || fx == PX_OCT || fx == PX_FREEZE) return KG_AMB;
    return KG_RED;
}
/* the small font (FONT_T, 6 x 11, anti-aliased: tools/gen_font.py): x, y the top of the capitals; returns the x after */
UI_SMALL static int32_t kg_t5(int32_t x, int32_t y, const char *s, uint16_t c) { return cv_text(x, y - 1, &FONT_T, s, c); }
UI_SMALL static int32_t kg_t5w(const char *s) { return text_w(&FONT_T, s) - 1; }
/* a label centred in a cell w wide at (x, y): as many of its letters as fit */
UI_SMALL static void kg_cell_label(int32_t x, int32_t y, int32_t w, const char *s, uint16_t c)
{
    char b[8];
    uint32_t n = str_len(s);
    if (n > 7u) n = 7u;
    str_cpy(b, s, n + 1u);
    while (n > 1u && kg_t5w(b) > w - 2)
        b[--n] = 0;
    kg_t5(x + (w - kg_t5w(b)) / 2, y, b, c);
}
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

/* the page: 0 DESTROY, FILTER, 1 DRUMS, 2 CHOP, 3 LOOP, 4 SLICE, 5 GRAIN, 6 SEQ, 7 BANK, 8 CHAOS, 9 SETUP */
#define KG_NPG 11u
enum { KP_DESTROY, KP_FILTER, KP_DRUMS, KP_CHOP, KP_LOOP, KP_SLICE, KP_GRAIN, KP_SEQ, KP_BANK, KP_CHAOS, KP_SETUP };
UI_SMALL static uint32_t kg_pageno(void)
{
    static const uint8_t ID[KG_NPG] = {G_KBANK, G_KFTYPE, G_KSTYLE, G_KDICE, G_KLREC, G_KLSL, G_KGPOS, G_KSTEP, G_KMULTI, G_KCMODE,
                                       G_KTGT};
    uint32_t id = cur_page()->id[0], i;
    for (i = 0; i < KG_NPG; i++)
        if (ID[i] == id) return i;
    return 0;
}

/* SEQ: the page of 16 steps shown: the one recording (or erasing) while PLAY runs, else the edited step's */
UI_SMALL static uint32_t kg_seqpage(void)
{
    uint32_t len = kg_seqlen(), n = len > 16u ? len / 16u : 1u;
    if (song.playing && (song.rec || kg_erase))
        return (kg.step / 16u) % n;
    return ((uint32_t)(song.g[G_KSTEP] - 1) / 16u) % n;
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
        int32_t fx = kg_bank_base() + (int32_t)i, lit = fx == now, d = fx - (int32_t)PUNCH_NFX;
        uint16_t c = kg_fam(fx);
        cv_rect(cx, cy, 25, 16, kg_mix(KG_PANEL, c, lit ? 256u : 170u));
        kg_cell_label(cx, cy + 5, 25, KG_SHORT[fx], KG_BG);
        if (lit) kg_box(cx - 1, cy - 1, 27, 18, KG_INK);
        if (song.g[G_KBANK]) {
            cv_rect(cx, cy + 17, 25, 2, KG_RULE);
            cv_rect(cx, cy + 17, (int32_t)kd_int[d % DD_N] / 4, 2, d == kg_sel ? KG_ACC : kg_mix(KG_RULE, KG_INK, 150));
            if (d == kg_sel) kg_box(cx - 2, cy - 2, 29, 22, KG_ACC);
        }
    }
}
/* LOOP, SLICE, GRAIN: the loop's waveform across the panel, its 16 slices; SLICE: what each plays (its number,
 * orange when moved, a bar under a reversed one, dots for a retrigger); GRAIN: the position and the spray; the
 * playhead (the transport's) white */
UI_SMALL static void kg_loopview(uint32_t pn)
{
    uint32_t c, i;
    int32_t x0 = 14, w = 212, ymid = 76;
    if (kl.ov_new == 2u) kl_outline();
    if (kl.st != KLS_HAS && kl.st != KLS_REC) {
        cv_text(x0 + 20, 62, &FONT_S, kl.st == KLS_ARMED ? "WAITING FOR THE BAR" : "NO LOOP: K1 REC", KG_MUTE);
        kg_t5(x0 + 20, 82, "(THE TEMPO DELAY LENDS ITS LINE)", KG_RULE);
        return;
    }
    for (c = 0; c < 110u; c++) {                   /* the outline, two pixels a column */
        int32_t h = kl.st == KLS_HAS ? kl.ov[c] : (c * kl.len / 110u < kl.w ? 10 : 1);
        uint16_t col = kl.st == KLS_REC ? KG_RED : kg_mix(KG_PANEL, KG_CYAN, 200);
        cv_rect(x0 + (int32_t)c * 2 - 1 + 1, ymid - h, 1, 2 * h + 1, col);
        cv_rect(x0 + (int32_t)c * 2 + 1, ymid - h / 2, 1, h + 1, kg_mix(KG_PANEL, col, 120));
    }
    for (i = 0; i <= KL_NS; i++)                    /* the slices */
        cv_rect(x0 + (int32_t)i * w / 16 - 1, 53, 1, i % 4u ? 4 : 6, KG_RULE);
    if (pn == KP_SLICE || (pn == KP_LOOP && song.g[G_KLMODE] == KL_SLICE))
        for (i = 0; i < KL_NS; i++) {
            int32_t sx = x0 + (int32_t)i * w / 16;
            char nb[4];
            uint16_t col = kl.map[i] != i ? KG_ACC : KG_MUTE;
            if ((int32_t)i + 1 == song.g[G_KLSL] && pn == KP_SLICE) {
                cv_rect(sx, 89, w / 16 - 1, 11, KG_ACC);
                col = KG_BG;
            }
            kg_u2a(nb, kl.map[i] + 1u);
            kg_t5(sx + (w / 16 - kg_t5w(nb)) / 2, 91, nb, col);
            if (kl.rev[i]) cv_rect(sx + 1, 102, w / 16 - 3, 2, KG_AMB);
            if (kl.ret[i] > 1u)
                for (c = 0; c < kl.ret[i] && c < 4u; c++) cv_rect(sx + 2 + (int32_t)c * 3, 106, 2, 2, KG_VIO);
        }
    if (pn == KP_GRAIN || (pn == KP_LOOP && song.g[G_KLMODE] == KL_GRAIN)) {   /* the position, the spray */
        int32_t gx = x0 + (kl.pad >= 0 ? kl.pad * w / 16 : (int32_t)kg_pct(G_KGPOS) * w / 100);
        int32_t sw = (int32_t)kg_pct(G_KCHAOS) * w / 800;
        cv_rect(gx, 90, sw + 2, 3, kg_mix(KG_PANEL, KG_VIO, 140));
        cv_rect(gx, 54, 2, 46, KG_VIO);
        kg_t5(x0, 104, "GRAINS", KG_VIO);
    }
    if (song.playing && kl.st == KLS_HAS && song.g[G_KLMODE] != KL_OFF) {   /* the playhead */
        uint32_t total = kl_bar_samples() * kl.bars, pos = ((clk_beat - kl.beat0) % (4u * kl.bars)) * beat_samples() + clk_samples();
        int32_t px = x0 + (int32_t)(total ? (pos / 64u) * (uint32_t)w / (total / 64u + 1u) : 0u);
        cv_rect(px, 53, 1, 50, KG_INK);
        if (kl.pad >= 0) cv_rect(x0 + kl.pad * w / 16, 53, w / 16, 2, KG_ACC);
    }
}

/* FILTER: its shape across the panel (a drawing of the response, not a measure: left low, right high), the note
 * marked, where MOVE takes it shaded; the types' colours as the effects' families */
UI_SMALL static void kg_filterview(void)
{
    int32_t t = song.g[G_KFTYPE], x0 = 14, w = 212, ytop = 54, h = 40, xc, x, prev = -1, mv = (int32_t)song.g[G_KFMOD];
    int32_t res = song.g[G_KFRES], pk = 4 + res * 18 / 127;
    uint16_t col = t == KF_OFF ? KG_RULE : t <= KF_NOTCH ? KG_CYAN : t == KF_DRIVE ? KG_RED : t <= KF_METAL ? KG_AMB : KG_VIO;
    xc = x0 + ((kf_anote ? (int32_t)kf_anote : song.g[G_KFCUT]) - 24) * w / 96;
    if (kf_amode & 15u) t = (int32_t)(kf_amode & 15u) - 1;
    col = t == KF_OFF ? KG_RULE : t <= KF_NOTCH ? KG_CYAN : t == KF_DRIVE ? KG_RED : t <= KF_METAL ? KG_AMB : KG_VIO;
    if (mv) cv_rect(clamp(xc - mv * w / 270, x0, x0 + w), ytop, clamp(mv * w / 135, 2, w), h + 2, kg_mix(KG_PANEL, col, 40));
    cv_rect(x0, ytop + h, w, 1, KG_RULE);
    for (x = 0; x < w; x++) {
        int32_t d = x0 + x - xc, a = d < 0 ? -d : d, y = 0, ph;   /* y: 0 the floor .. h the top */
        switch (t) {
        case KF_LP: case KF_DRIVE: y = d < 0 ? 26 : 26 - d * 26 / 40; y += a < 8 ? pk * (8 - a) / 8 : 0; break;
        case KF_HP: y = d > 0 ? 26 : 26 + d * 26 / 40; y += a < 8 ? pk * (8 - a) / 8 : 0; break;
        case KF_BP: y = 8 + (a < 30 ? (30 - a) * (14 + pk) / 30 : 0); break;
        case KF_NOTCH: y = a < 14 ? 26 - (14 - a) * 26 / 14 : 26; break;
        case KF_COMBP: case KF_COMBN: case KF_METAL: case KF_RESON: {
            int32_t per = t == KF_METAL ? 11 + (x * 7 / w) : 22, m = (x - (xc - x0) + 1000 * per) % per;   /* (teeth) */
            if (t == KF_COMBN) m = (m + per / 2) % per;
            if (t == KF_RESON) y = (m < 2 && x > xc - x0 - 2 && x < xc - x0 + 70) ? 34 : 4;
            else y = 6 + (m < per / 2 ? (per / 2 - m) : (m - per / 2)) * (10 + pk) / (per / 2) * (t == KF_METAL ? (1 + (x * 13) % 3) : 3) / 3;
            break;
        }
        case KF_VOWEL: {
            int32_t f2 = d - 38;
            y = 6 + (a < 14 ? (14 - a) * (14 + pk / 2) / 14 : 0) + (f2 > -12 && f2 < 12 ? (12 - (f2 < 0 ? -f2 : f2)) * (10 + pk / 2) / 12 : 0);
            break;
        }
        case KF_PHASER:
            ph = (x * 4 * 65536 / w) - (xc - x0) * 300;
            y = 26 - (sine_i((uint32_t)ph << 14) > 26000 ? 18 : 0) - (sine_i((uint32_t)ph << 14) > 31000 ? 6 : 0);
            break;
        default: y = 20; break;
        }
        y = clamp(y, 0, h);
        if (prev >= 0) cv_line(x0 + x - 1, ytop + h - prev, x0 + x, ytop + h - y, col);
        cv_rect(x0 + x, ytop + h - y + 1, 1, y, kg_mix(KG_PANEL, col, 50));
        prev = y;
    }
    if (t != KF_OFF) cv_rect(xc, ytop, 1, h, KG_INK);
    else cv_text(x0 + 70, ytop + 12, &FONT_S, "FILTER OFF", KG_MUTE);
}

UI_SMALL static void kg_paint(void)
{
    static const char *const TITLE[KG_NPG] = {"DRUM DESTROYER", "FILTER", "DRUMS", "CHOP / MUTATE", "LOOP SAMPLER", "SLICES",
                                              "GRAINS", "EFFECT SEQUENCE", "PAD MODES", "MODES", "SETUP / MPC"};
    static const char *const OUT[3] = {"FM-1", "MPC", "BOTH"};
    static const char *const HINT[KG_NPG] = {"ALGO FX / PRESETS INTENS / OCT+ DICE", "KEYS TUNE / REC: AUTOMATION",
                                             "K4 GENERATE / KEYS PLAY THE DRUMS",
                                             "K1 DICE / K2 MUTATE / K3 RESTORE", "K1 REC: NEXT BAR / KEYS: SLICES",
                                             "KEYS: SLICES / OCT+ DICE / OCT- ORDER", "KEYS: POSITION / CHAOS: SPRAY",
                                             "KEY STEP / OCT+ DICE / OCT- CLEAR", "K1 SOLO OR MULTI / K3 CLEAN: ORIGINAL",
                                             "AUTO + LFO + MUTATIONS ON THE BAR", "OUT: FM-1, MPC, BOTH / CH: THE MPC'S"};
    static const char *const SRC[4] = {"", "KEY", "SEQ", "AUTO"};
    const page_t *pg = cur_page();
    uint32_t pn = kg_pageno(), i, out = (uint32_t)song.g[G_KTGT] % 3u;
    int32_t now = kg.now, play = song.playing ? (int32_t)kg.step : -1, cur = pn == KP_SEQ ? (int32_t)song.g[G_KSTEP] - 1 : -1, x;
    char s[48], k[64];
    cv_rect(0, 0, 240, 240, KG_BG);
    ribbon_bar(1, (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u);   /* (ui_draw.c: the family's pages as tabs) */
    /* the card: title, the orange line under it (or the last message), the tags */
    kg_rbox(3, 19, 234, 219, KG_CARD, kg_mix(KG_CARD, KG_ACC, 150));
    {   /* (DESTROY: the bank on the pads names it) */
        const char *t = pn == KP_DESTROY ? (song.g[G_KBANK] >= 2 ? "BEAT FX" : song.g[G_KBANK] ? TITLE[0] : "SLOOP FX") : TITLE[pn];
        cv_text(10, 21, &FONT_S, t, KG_INK);
        cv_text(11, 21, &FONT_S, t, KG_INK);           /* (bold) */
    }
    s[0] = 0;
    if (ui.msg_t) str_cpy(s, ui.msg, sizeof s);
    else switch (pn) {
    case KP_DESTROY:
        str_cpy(s, DD_NAME[kg_sel % DD_N], sizeof s);
        str_cpy(s + str_len(s), " / INTENS ", sizeof s - str_len(s));
        kg_u2a(s + str_len(s), kd_int[kg_sel % DD_N]);
        str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
        break;
    case KP_FILTER: {
        static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        uint32_t nt = (uint32_t)song.g[G_KFCUT];
        str_cpy(s, N_KFTYPE[song.g[G_KFTYPE] % KF_N], sizeof s);
        str_cpy(s + str_len(s), " / ", sizeof s - str_len(s));
        str_cpy(s + str_len(s), NN[nt % 12u], sizeof s - str_len(s));
        kg_u2a(s + str_len(s), nt / 12u - 1u);
        str_cpy(s + str_len(s), " / RESO ", sizeof s - str_len(s));
        kg_u2a(s + str_len(s), (uint32_t)song.g[G_KFRES] * 100u / 127u);
        str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
        break;
    }
    case KP_DRUMS:
        str_cpy(s, N_KSTYLE_P[song.g[G_KSTYLE] % KS_N], sizeof s);
        str_cpy(s + str_len(s), " / KIT ", sizeof s - str_len(s));
        str_cpy(s + str_len(s), DRUM_KIT_NAMES[drum_kit()], sizeof s - str_len(s));
        break;
    case KP_CHOP:
        if (kr.mutated && kr.recipe[0]) {
            str_cpy(s, "DICE: ", sizeof s);
            str_cpy(s + str_len(s), kr.recipe, sizeof s - str_len(s));
        } else if (kr.mutated) {
            str_cpy(s, "MUTATED / ", sizeof s);
            kg_u2a(s + str_len(s), kr.changed);
            str_cpy(s + str_len(s), " CHANGES", sizeof s - str_len(s));
        } else
            str_cpy(s, kr.has ? "THE ORIGINAL" : "NOTHING MUTATED YET", sizeof s);
        break;
    case KP_LOOP:
    case KP_SLICE:
    case KP_GRAIN:
        if (kl.st == KLS_EMPTY) str_cpy(s, "EMPTY / K1 REC: RECORD WHAT PLAYS", sizeof s);
        else if (kl.st == KLS_ARMED) str_cpy(s, "REC ARMED: ON THE NEXT BAR", sizeof s);
        else if (kl.st == KLS_REC) {
            str_cpy(s, "RECORDING ", sizeof s);
            kg_u2a(s + str_len(s), kl.len ? kl.w * 100u / kl.len : 0u);
            str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
        } else {
            static const char *const M[4] = {"OFF", "LOOP", "SLICE", "GRAIN"};
            kg_u2a(s, kl.bars);
            str_cpy(s + str_len(s), kl.bars > 1u ? " BARS / " : " BAR / ", sizeof s - str_len(s));
            str_cpy(s + str_len(s), M[(uint32_t)song.g[G_KLMODE] & 3u], sizeof s - str_len(s));
            str_cpy(s + str_len(s), song.g[G_KLSRC] ? " / MIX" : " / TRACKS MUTED", sizeof s - str_len(s));
        }
        break;
    case KP_SEQ: {
        uint32_t n = 0, len = kg_seqlen();
        if (song.playing && song.rec) {
            str_cpy(s, "REC: PLAY THE PADS", sizeof s);
            break;
        }
        for (i = 0; i < len; i++) n += kg.seq[i] >= 0;
        if (!len) { str_cpy(s, "SEQ OFF", sizeof s); break; }
        str_cpy(s, "PAGE ", sizeof s);
        kg_u2a(s + str_len(s), kg_seqpage() + 1u);
        str_cpy(s + str_len(s), "/", sizeof s - str_len(s));
        kg_u2a(s + str_len(s), len / 16u);
        str_cpy(s + str_len(s), " / ", sizeof s - str_len(s));
        kg_u2a(s + str_len(s), n);
        str_cpy(s + str_len(s), " FX / INT ", sizeof s - str_len(s));
        {
            uint32_t si = kg.sint[(uint32_t)(song.g[G_KSTEP] - 1) % KG_STEPS];
            if (si) { kg_u2a(s + str_len(s), si - 1u); str_cpy(s + str_len(s), "%", sizeof s - str_len(s)); }
            else str_cpy(s + str_len(s), "--", sizeof s - str_len(s));
        }
        break;
    }
    case KP_BANK:
        str_cpy(s, song.g[G_KBANK] >= 2 ? "BEAT / " : song.g[G_KBANK] ? "DRUM DESTROYER / " : "SLOOP FX / ", sizeof s);
        str_cpy(s + str_len(s), DD_NAME[kg_sel % DD_N], sizeof s - str_len(s));
        break;
    case KP_CHAOS:
        str_cpy(s, song.g[G_KCMODE] ? "CHAOS MODE ON / " : "CHAOS MODE OFF / ", sizeof s);
        kg_u2a(s + str_len(s), (uint32_t)song.g[G_KCHAOS]);
        str_cpy(s + str_len(s), "%", sizeof s - str_len(s));
        break;
    default:
        str_cpy(s, "OUT ", sizeof s);
        str_cpy(s + str_len(s), OUT[out], sizeof s - str_len(s));
        str_cpy(s + str_len(s), " / MPC CH ", sizeof s - str_len(s));
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
        if (kg_erase) { tx -= kg_t5w("CLEAR") + 12; kg_tag(tx, 24, "CLEAR", KG_INK, KG_BG); }
        else if (song.playing && song.rec) { tx -= kg_t5w("REC") + 12; kg_tag(tx, 24, "REC", KG_RED, KG_INK); }
        if ((song.g[G_KLFO] || song.g[G_KCMODE]) && tx > 150) { tx -= kg_t5w("LFO") + 12; kg_tag(tx, 24, "LFO", KG_VIO, KG_BG); }
        if (song.g[G_KMULTI] && tx > 160) { tx -= kg_t5w("MULTI") + 12; kg_tag(tx, 24, "MULTI", KG_INK, KG_BG); }
    }
    /* the graph panel: the pads, the drums or the effect sequence; the effect playing and the mix under it */
    kg_rbox(9, 49, 222, 76, KG_PANEL, KG_PANEL);
    if (pn == KP_DRUMS || pn == KP_CHOP || pn == KP_CHAOS)
        kg_drumgrid(55);
    else if (pn == KP_FILTER)
        kg_filterview();
    else if (pn == KP_LOOP || pn == KP_SLICE || pn == KP_GRAIN)
        kg_loopview(pn);
    else if (pn == KP_SEQ)
        for (i = 0; i < 16u; i++) {                   /* the page of 16 steps shown (kg_seqpage), each with its INTENSITY */
            uint32_t at = kg_seqpage() * 16u + i;
            int32_t cx = 13 + (int32_t)(i & 7u) * 27, cy = 53 + (int32_t)(i >> 3) * 22, fx = kg.seq[at], lit = (int32_t)at == play;
            if (at >= kg_seqlen()) { kg_box(cx, cy, 25, 18, KG_RULE); continue; }
            if (fx >= 0 && fx < (int32_t)KG_NFX) {
                uint16_t c = kg_fam(fx);
                cv_rect(cx, cy, 25, 18, kg_mix(KG_PANEL, c, lit ? 256u : 180u));
                kg_cell_label(cx, cy + 6, 25, KG_SHORT[fx], KG_BG);
            } else {
                kg_box(cx, cy, 25, 18, kg_mix(KG_PANEL, KG_RULE, 170));
                cv_rect(cx + 11, cy + 8, 2, 2, (i & 3u) == 0u ? KG_MUTE : KG_RULE);
            }
            if (kg.sint[at]) cv_rect(cx, cy + 16, (int32_t)(kg.sint[at] - 1u) / 4, 2, KG_INK);   /* (recorded INTENSITY) */
            if (kg.fnote[at])                           /* (the FILTER automated on the step: a violet tick) */
                cv_rect(cx + 1, cy + 14, 3, 3, KG_VIO);
            if (kg.seq2[at] >= 0)                       /* (a second effect on the step: its colour, a corner) */
                for (x = 0; x < 6; x++) cv_rect(cx + 19 + x, cy, 6 - x, 1, kg_fam(kg.seq2[at])), cv_rect(cx + 19 + x, cy + x, 1, 1, KG_INK);
            if (kg.prob[at] < 100u && fx >= 0) {        /* (a probability: a white bar along the top, its share) */
                cv_rect(cx, cy, 25, 2, KG_PANEL);
                cv_rect(cx, cy, (int32_t)kg.prob[at] / 4, 2, KG_INK);
            }
            if (lit) kg_box(cx - 1, cy - 1, 27, 20, kg_erase ? KG_RED : KG_INK);
            if ((int32_t)at == cur) cv_rect(cx + 3, cy + 19, 19, 2, KG_ACC);
        }
    else
        kg_pads16(now);
    {
        int32_t ok = now >= 0 && now < (int32_t)KG_NFX;
        const char *n = ok ? kg_name(now) : song.playing ? "READY" : "STOPPED";
        if (pn == KP_FILTER) {                         /* FILTER: its automation, the page of 16 steps playing */
            uint32_t pg0 = song.playing ? (kg.step / 16u) * 16u : 0u, st;
            for (st = 0; st < 16u; st++) {
                uint32_t at = pg0 + st, fm = kg.fmode[at];
                int32_t bx = 14 + (int32_t)st * 13, hh = kg.fnote[at] ? 2 + ((int32_t)kg.fnote[at] - 24) * 16 / 96 : 0;
                uint16_t cc = (fm & 15u) <= 1u ? KG_RULE : (fm & 15u) <= 5u ? KG_CYAN : (fm & 15u) == 6u ? KG_RED
                                                         : (fm & 15u) <= 9u ? KG_AMB : KG_VIO;
                cv_rect(bx, 119, 11, 1, KG_RULE);
                if (hh) cv_rect(bx, 119 - hh, 11, hh, cc);
                if (song.playing && at == kg.step) cv_rect(bx, 101, 11, 1, KG_INK);
            }
        } else if (pn == KP_LOOP || pn == KP_SLICE || pn == KP_GRAIN) {   /* (the loop view uses the panel) */
        } else if (kg.nfx > 1u) {                      /* MULTI: the effects stacked, named */
            x = 14;
            for (i = 0; i < 4u && kg.list[i] >= 0; i++) {
                if (i) x = kg_t5(x + 2, 99, "+", KG_MUTE);
                x = kg_t5(x + 2, 99, KG_SHORT[kg.list[i]], kg_fam(kg.list[i]));
            }
        } else {
            x = kg_t5(14, 99, n, ok ? kg_fam(now) : KG_MUTE);
            if (ok) kg_t5(x + 6, 99, SRC[kg.src & 3u], KG_MUTE);
        }
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
                if (k[0]) str_cpy(k + str_len(k), form < 2u ? " / " : " ", sizeof k - str_len(k));
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
    for (i = 0; i < KG_STEPS; i++) h = (h ^ (uint32_t)(kg.seq[i] + 2) ^ (uint32_t)kg.sint[i] << 8) * 16777619u;
    h ^= (uint32_t)kg_erase * 0x45D9F3Bu + (uint32_t)(song.rec != 0) * 0x119DE1F3u;
    h = str_hash(h, kr.recipe);
    h ^= (song.playing ? kg.step + 1u : 0u) * 977u + (uint32_t)(kg.now + 2) * 65537u + kg.src * 31u + ui.page * 7919u;
    for (i = G_KCHAOS; i < G_COUNT; i++) h = (h ^ (uint32_t)song.g[i]) * 16777619u;
    for (i = 0; i < DD_N; i++) h = (h ^ kd_int[i]) * 16777619u;
    h ^= (song.playing ? (clk_beat & 3u) + 1u : 0u) * 0x2545F491u;
    h ^= (uint32_t)song.g[G_BPM] * 104729u + (ui.hot_t ? ui.hot_col + 1u : 0u) * 13u + kg_sel * 7u + (uint32_t)song.g[G_SWING];
    for (i = 0; i < KL_NS; i++) h = (h ^ (uint32_t)kl.map[i] ^ (uint32_t)kl.rev[i] << 5 ^ (uint32_t)kl.ret[i] << 6) * 16777619u;
    for (i = 0; i < KG_STEPS; i++)
        h = (h ^ (uint32_t)(kg.seq2[i] + 2) ^ (uint32_t)kg.prob[i] << 8 ^ (uint32_t)kg.fnote[i] << 16 ^ (uint32_t)kg.fmode[i] << 24) * 16777619u;
    h ^= (uint32_t)kf_anote * 0x01000193u;
    for (i = 0; i < 4u; i++) h = (h ^ (uint32_t)(kg.list[i] + 2)) * 16777619u;
    h ^= kl.st * 0x3C6EF372u + kl.w / 2048u * 0x85EBCA6Bu + (uint32_t)(kl.pad + 2) * 0x27D4EB2Fu + kl.ov_new;
    if (kg_pageno() >= KP_LOOP && kg_pageno() <= KP_GRAIN && song.playing && kl.st == KLS_HAS)   /* (the playhead moves) */
        h ^= (clk_beat * 8u + clk_pos / ((uint32_t)FS * 60u / 8u)) * 0x9E3779B1u;
    if (kg_pageno() == KP_FILTER && song.playing)       /* (the band's playhead) */
        h ^= (kg.step + 1u) * 0x61C88647u;
    if (kg_pageno() == KP_DRUMS || kg_pageno() == KP_CHOP || kg_pageno() == KP_CHAOS)   /* the drum grid: the pattern, the 16th playing */
        h ^= kr_sig() + (song.playing ? TDRUM->seq_idx + 1u : 0u) * 31337u + kr.changed + (uint32_t)TDRUM->p[P_E0] * 7u;
    if (ui.msg_t) h = str_hash(h ^ 0x9E37u, ui.msg);
    return h;
}

UI_SMALL static void kg_screen_draw(void)
{
    uint32_t sig, pn = kg_pageno();
    /* the keys: SEQ's steps, the drums (DRUMS), the loop's slices (LOOP, SLICE, GRAIN), else the pads */
    kg_pads = (uint8_t)(pn == KP_SEQ ? 2u : pn == KP_DRUMS ? 0u : pn >= KP_LOOP && pn <= KP_GRAIN ? 3u : pn == KP_FILTER ? 4u : 1u);
    if (kg_keynote) {                                   /* FILTER: a key tuned FREQ to its note */
        song.g[G_KFCUT] = (int16_t)clamp(kg_keynote - 1, 24, 120);
        kg_keynote = 0;
        kf_touch = 1;
    }
    if (kg_keystep) {                                   /* SEQ: a white key picked the step (on the page shown) */
        uint32_t at = kg_seqpage() * 16u + kg_keystep - 1u;
        song.g[G_KSTEP] = (int16_t)(at + 1u);
        kg_keystep = 0;
    }
    {   /* what the knobs show follows what changed elsewhere (REC, DICE): the step's, the slice's */
        static const uint8_t RI[9] = {0, 0, 1, 2, 3, 3, 3, 3, 4};
        uint32_t at = (uint32_t)(song.g[G_KSTEP] - 1) % KG_STEPS, sl = (uint32_t)(song.g[G_KLSL] - 1) % KL_NS;
        song.g[G_KSFX] = (int16_t)(kg.seq[at] + 1);
        song.g[G_KPROB] = kg.prob[at];
        song.g[G_KLPLAY] = (int16_t)(kl.map[sl] + 1u);
        song.g[G_KLREV] = kl.rev[sl];
        song.g[G_KLRET] = RI[kl.ret[sl] % 9u];
    }
    if (song.g[G_KBANK] && (kg_sel >= DD_BANK) != (song.g[G_KBANK] >= 2))   /* (the selection: in the bank shown) */
        kg_sel = (uint8_t)(kg_sel % DD_BANK + (song.g[G_KBANK] >= 2 ? DD_BANK : 0u));
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
    if ((ui.frame & 1u) && !(pn >= KP_LOOP && pn <= KP_GRAIN) && pn != KP_FILTER)   /* (their own views) */
        kg_scope();
}

/* ---- K-GLITCH DRUMS (kg_drum.c): EDIT on the drum track. Its values are the sound's (the lane last played, SOUND):
 * shown each frame (kgd_ui_sync), written back when a knob turns (kgd_ui_set, ui_input.c), heard at once (stopped) */
static const uint8_t KGD_GID[KGD_NK][KGD_NP] = {
    {G_KMPIT, G_KMRAT, G_KMIDX, G_KMIDEC, G_KMFB, G_KMBEND, G_KMBT, G_KMDEC, G_KMNZ, G_KMNDEC, G_KMNCOL, G_KDLVL},
    {G_K6PIT, G_K6W1, G_K6FR, G_K6W2, G_K6SWP, G_K6PW, G_K6PWM, G_K6DEC, G_K6FLT, G_K6CUT, G_K6RES, G_KDLVL}};
static int32_t kgd_model(void)                      /* the drum track's K-GLITCH kit (KGD_*), -1 another kit */
{
    int32_t k = TDRUM->p[P_E0] - (int32_t)DRUM_KGFM;
    return k >= 0 && k < (int32_t)KGD_NK ? k : -1;
}
static int32_t kgd_bias(uint32_t m, uint32_t k) { return m == KGD_C64 && (k == C6_SWP || k == C6_PWM) ? 64 : 0; }
UI_SMALL static void kgd_ui_sync(void)
{
    int32_t m = kgd_model();
    uint32_t k;
    song.g[G_KDKIT] = (int16_t)(m + 1);
    song.g[G_KDSND] = (int16_t)(pen_lane & 15u);
    song.g[G_KTFLT] = TSEL->p[P_TFLT];                /* (DJ FILTER: the selected track's) */
    song.g[G_KTRES] = song.g[G_KTR1 + song.sel % 4u];
    if (m < 0)
        return;
    {
        const kgd_snd_t *d = kgd_snd((uint32_t)m, pen_lane);
        for (k = 0; k < KGD_NP; k++)
            song.g[KGD_GID[m][k]] = (int16_t)(d->p[k] - kgd_bias((uint32_t)m, k));
        song.g[G_KXSAT] = d->sat, song.g[G_KXDRV] = d->drv, song.g[G_KXDLY] = d->dly, song.g[G_KXREV] = d->rev;
    }
}
UI_SMALL static void kgd_ui_set(uint32_t id, int32_t v)
{
    int32_t m = kgd_model();
    uint32_t k;
    kgd_snd_t *d;
    if (id == G_KDKIT) {                              /* the drum track's kit: FM DRUM, C64 DRUM, or back to the 808 */
        if (v > 0 || m >= 0)
            TDRUM->p[P_E0] = (int16_t)(v > 0 ? DRUM_KGFM + (uint32_t)v - 1u : DRUM_DEFAULT_KIT);
        ui.force = 1;
        return;
    }
    if (id == G_KTFLT || id == G_KTRES) {             /* DJ FILTER: the selected track's FILTER, its resonance */
        if (id == G_KTRES) {
            song.g[G_KTR1 + song.sel % 4u] = (int16_t)clamp(v, 0, 100);
            return;
        }
        TSEL->p[P_TFLT] = (int16_t)clamp(v, -64, 63);
        if (song.playing && ((song.rec >> song.sel) & 1u)) {   /* (REC: written on the step playing, as a sound knob) */
            fm1_irq_off();
            if (!lock_set(TSEL, TSEL->seq_idx % trk_len(TSEL), P_TFLT, v))
                ui_message("AUTOMATION FULL (24)");
            fm1_irq_on();
        }
        return;
    }
    if (id == G_KDSND) {
        pen_lane = (uint8_t)(v & 15);
        kgd_aud_lane = pen_lane, kgd_aud_vel = 100;   /* (heard: the sound picked) */
        return;
    }
    if (m < 0)
        return;
    d = kgd_snd((uint32_t)m, pen_lane);
    for (k = 0; k < KGD_NP; k++)
        if (KGD_GID[m][k] == id)
            d->p[k] = (uint8_t)clamp(v + kgd_bias((uint32_t)m, k), KGD_MIN[m][k], KGD_MAX[m][k]);
    if (id == G_KXSAT) d->sat = (uint8_t)(v & 3);
    if (id == G_KXDRV) d->drv = (uint8_t)(v & 127);
    if (id == G_KXDLY) d->dly = (uint8_t)(v & 127);
    if (id == G_KXREV) d->rev = (uint8_t)(v & 127);
    if (id == G_KDINIT) {
        *d = KGD_DEF[m][pen_lane & 15u];
        ui_say("INIT ", N_KDSND[pen_lane & 15u]);
    }
    if (!song.playing)
        kgd_aud_lane = pen_lane, kgd_aud_vel = 100;   /* (heard at once) */
}
