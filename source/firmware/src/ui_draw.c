/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Felucca UI drawing: status bar (top), columns + gauges, graphs, focus readout,
 * footer (steps + engine / preset / page). */
static void draw_menu(void);
static uint32_t str_hash(uint32_t h, const char *s);

/* --------------------------------------------------------- drawing --- */
UI_SMALL static int is_eng_name(const char *s)                 /* one of the ENGINES[]->name strings */
{
    uint32_t e;
    for (e = 0; e < NENGINES; e++)
        if (s == ENGINES[e]->name)
            return 1;
    return 0;
}

/* at most 5 characters, and no wider than maxw */
UI_SMALL static void fit(char *d, const char *src, const felucca_font_t *f, int32_t maxw)
{
    str_cpy(d, src, is_eng_name(src) ? 8 : 6);           /* engine names are kept whole */
    while (d[0] && text_w(f, d) > maxw)
        d[str_len(d) - 1u] = 0;
}

UI_SMALL static int32_t batt_level(void)                         /* ADC ch3 thresholds */
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}
/* charging (a USB host powers us; there is no charger status line): bars fill 1, 2, 3
 * every 0.6 s, so the header is redrawn twice a second at most; else the level */
UI_SMALL static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 1 + (int32_t)((fm1_ms / 600u) % 3u);
    return batt_level();
}

/* top bar: transport, BPM, octave | USB, battery, CPU; messages replace it */
/* K-GLITCH (the RIBBON palette): the tab bar of every page (SLOOP's and K-GLITCH's own, kg_ui.c), its top at y in the
 * canvas: the pages of the family as tabs, the one shown an orange pill in the middle of as many of its neighbours
 * as fit (a triangle where more go on, left or right); at the right the tempo, then the battery's charge, small (USB:
 * powered). A message takes the tabs' place */
UI_SMALL static void ribbon_bar(int32_t y, uint32_t rec)
{
    const page_t *pg = cur_page();
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    char b[16], bt[8];
    const char *tl[24];
    int32_t w[24], x, xr, lo, hi, used;
    uint32_t i, n = 0, cur = 0;
    if (ui.home) {
        tl[n++] = "HOME";
    } else {
        for (i = 0; i < NPAGES && n < 24u; i++)
            if (PAGES[i].fam == pg->fam && (i == ui.page || (page_shown(&PAGES[i]) && PAGES[i].scope != SC_SONG &&
                                                             PAGES[i].scope != SC_DRUM))) {
                const char *pt = PAGES[i].scope == SC_ENGINE ? e->page_title[PAGES[i].id[0] != P_E0] : 0;
                if (i == ui.page) cur = n;
                tl[n++] = pt ? pt : PAGES[i].title;
            }
    }
    /* the right: the battery's charge (or USB), then the tempo before it */
    if (usb.config && !usb.suspended)
        str_cpy(bt, "USB", sizeof bt);
    else
        fmt_int(bt, clamp(((int32_t)song.batt_raw - 501) * 100 / 120, 0, 100));
    if (!song.batt_raw && !(usb.config && !usb.suspended))
        bt[0] = 0;                                    /* (not measured yet: nothing) */
    xr = 238 - text_w(&FONT_T, bt);
    cv_text(xr, y + 2, &FONT_T, bt, batt_level() <= 0 && !(usb.config && !usb.suspended) ? TE_RED : C_DIM);
    fmt_int(b, song.g[G_BPM]);
    xr -= text_w(&FONT_T, b) + 6;
    cv_text(xr, y + 2, &FONT_T, b, ui.bpm_t ? C_WHITE : C_AMB);
    if (song.playing)                                 /* (playing: a small triangle) */
        for (i = 0; i < 5u; i++) cv_rect(xr - 7 + (int32_t)i, y + 4 + (int32_t)i / 2, 1, 7 - (int32_t)(i / 2u) * 2, C_WHITE);
    if (rec) te_disc(xr - 13, y + 8, 3, rec == 2u ? TE_RED : C_GRAY);
    if (mlu.on) { xr -= 34; cv_text(xr, y + 2, &FONT_T, "LEARN", C_WHITE); }
    else if (song.octave) {
        str_cpy(b, song.octave > 0 ? "+" : "", 4);
        fmt_int(b + str_len(b), song.octave);
        xr -= 16 + text_w(&FONT_T, b);
        cv_text(xr, y + 2, &FONT_T, b, C_WHITE);
    }
    xr -= rec || song.playing ? 18 : 6;
    if (ui.msg_t) {
        cv_text(4, y + 2, &FONT_T, ui.msg, C_WHITE);
        return;
    }
    /* the tabs: from the one shown, outwards, while they fit between 7 and xr - 7 */
    for (i = 0; i < n; i++) w[i] = text_w(&FONT_T, tl[i]) + 10;
    lo = hi = (int32_t)cur;
    used = w[cur];
    for (;;) {
        int grew = 0;
        if (hi + 1 < (int32_t)n && used + w[hi + 1] + 2 <= xr - 14) { hi++; used += w[hi] + 2; grew = 1; }
        if (lo > 0 && used + w[lo - 1] + 2 <= xr - 14) { lo--; used += w[lo] + 2; grew = 1; }
        if (!grew) break;
    }
    x = 7;
    if (lo > 0)                                       /* more on the left */
        for (i = 0; i < 4u; i++) cv_rect(1 + (int32_t)i, y + 7 - (int32_t)i, 1, 1 + 2 * (int32_t)i, C_GRAY);
    if (hi + 1 < (int32_t)n)                          /* more on the right */
        for (i = 0; i < 4u; i++) cv_rect(xr - 5 + (int32_t)i, y + 4 + (int32_t)i, 1, 7 - 2 * (int32_t)i, C_GRAY);
    for (i = (uint32_t)lo; i <= (uint32_t)hi; i++) {
        if (i == cur) cv_card(x, y + 1, w[i], 14, C_WHITE, C_WHITE);
        cv_text(x + 5, y + 2, &FONT_T, tl[i], i == cur ? C_BLACK : C_GRAY);
        x += w[i] + 2;
    }
}
UI_SMALL static void draw_head_ribbon(uint32_t rec)
{
    cv_begin(240, H_HEAD, C_BLACK);
    ribbon_bar(2, rec);
    cv_blit(0, Y_HEAD);
}
UI_SMALL static void draw_head(void)
{
    char b[16];
    uint32_t i;
    int32_t x;
    uint32_t rec = (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u;   /* 2 the selected track armed, 1 another */
    uint32_t sig = (uint32_t)song.playing * 3u + rec * 5u + (uint32_t)(song.octave + 8) * 11u + song.sel * 13131u +
                   (ui.msg_t ? str_hash(7u, ui.msg) : 0u) + (uint32_t)song.g[G_BPM] * 101u + (ui.bpm_t != 0) * 31u +
                   (uint32_t)batt_shown() * 7777u + (usb.config && !usb.suspended) * 99991u + mlu.on * 7919u;
    if (pal_ribbon)                                   /* (the tabs: the page; the battery's figure) */
        sig += (uint32_t)ui.page * 2654435761u + ui.home * 977u + (uint32_t)song.batt_raw * 40503u;
    if (!ui.force && sig == ui.head_sig)
        return;
    ui.head_sig = sig;
    if (pal_ribbon) {
        draw_head_ribbon(rec);
        return;
    }
    cv_begin(240, H_HEAD, C_BLACK);
    if (ui.msg_t) {
        cv_text(4, 1, &FONT_S, ui.msg, C_HI);
        cv_blit(0, Y_HEAD);
        return;
    }
    if (song.playing) {                               /* > play, square stop */
        for (i = 0; i < 5u; i++)
            cv_rect(4 + (int32_t)i * 2, 4 + (int32_t)i, 2, 10 - 2 * (int32_t)i, C_WHITE);
    } else {
        cv_rect(4, 5, 8, 8, C_HI);
    }
    if (rec)                                          /* recording armed: white = this track, gray = another */
        te_disc(21, 9, 4, rec == 2u ? TE_RED : C_GRAY);
    fmt_int(b, song.g[G_BPM]);
    x = 32;
    if (FELUCCA_ICONS) {                              /* metronome, then the BPM */
        cv_icon(x, 2, ICON_TEMPO, C_GRAY);
        x += 14;
    }
    x = cv_text(x, 1, &FONT_S, b, ui.bpm_t ? C_WHITE : C_HI);   /* white while SELECT turns it */
    if (mlu.on) {                                     /* MIDI LEARN on (2.6, GLO + F#3): in the octave's place */
        cv_text(x + 12, 1, &FONT_S, "LEARN", C_AMB);
    } else if (song.octave) {
        str_cpy(b, song.octave > 0 ? "+" : "", 4);
        fmt_int(b + str_len(b), song.octave);
        cv_text(x + 12, 1, &FONT_S, "OCT", C_GRAY);
        cv_text(x + 40, 1, &FONT_S, b, C_HI);
    }
    if (FELUCCA_ICONS) {                              /* the selected track: tape + number */
        cv_icon(156, 2, ICON_TAPE, C_GRAY);
        b[0] = (char)('1' + song.sel);
        b[1] = 0;
        cv_rect(168, 2, 12, 16, TE_COL[song.sel & 3u]);
        cv_text(170, 1, &FONT_S, b, C_BLACK);
    } else {
        b[0] = 'T';
        b[1] = (char)('1' + song.sel);
        b[2] = 0;
        cv_text(158, 1, &FONT_S, b, C_HI);
    }
    {   /* battery, 3 bars; USB when a host is there */
        int32_t lvl = batt_shown(), k;
        int32_t bx = 236 - 19;                          /* right edge (the CPU figure is in the console) */
        cv_rect(bx, 4, 17, 1, C_GRAY);
        cv_rect(bx, 12, 17, 1, C_GRAY);
        cv_rect(bx, 4, 1, 9, C_GRAY);
        cv_rect(bx + 16, 4, 1, 9, C_GRAY);
        cv_rect(bx + 17, 6, 2, 5, C_GRAY);
        for (k = 0; k < lvl; k++)
            cv_rect(bx + 2 + k * 5, 6, 3, 5, lvl == 1 && batt_level() <= 1 ? C_WHITE : C_HI);
        if (usb.config && !usb.suspended)
            cv_text(bx - 28, 1, &FONT_S, "USB", C_DIM);
    }
    cv_blit(0, Y_HEAD);
}
/* full redraw: the strips (head, columns, graph, foot) cover the rest, so only
 * the space between them is cleared, then the rules are drawn */
UI_SMALL static void draw_frame(void)
{
    uint32_t i;
    if (pal_ribbon) {                                   /* (K-GLITCH: no column rules; orange under the tabs, over the foot) */
        lcd_fill(0, H_HEAD, 240, Y_LABEL - H_HEAD, C_BLACK);
        lcd_fill(0, Y_SEP_END, 240, Y_GRAPH - Y_SEP_END, C_BLACK);
        lcd_fill(0, Y_GRAPH + H_GRAPH, 240, Y_FOOT - Y_GRAPH - H_GRAPH, C_BLACK);
        lcd_fill(0, Y_LABEL, 2, Y_SEP_END - Y_LABEL, C_BLACK);
        lcd_fill(238, Y_LABEL, 2, Y_SEP_END - Y_LABEL, C_BLACK);
        for (i = 1; i < 4u; i++)
            lcd_fill(i * 60u - 2u, Y_LABEL, 4, Y_SEP_END - Y_LABEL, C_BLACK);
        lcd_fill(0, H_HEAD + 1, 240, 1, C_WHITE);
        lcd_fill(0, Y_FOOT - 2, 240, 1, C_WHITE);
        return;
    }
    lcd_fill(0, H_HEAD, 240, Y_LABEL - H_HEAD, C_BLACK);
    lcd_fill(0, Y_SEP_END, 240, Y_GRAPH - Y_SEP_END, C_BLACK);
    lcd_fill(0, Y_GRAPH + H_GRAPH, 240, Y_FOOT - Y_GRAPH - H_GRAPH, C_BLACK);
    for (i = 0; i < 4u; i++)                            /* left inset of each column */
        lcd_fill(i * 60u, Y_LABEL, 4, Y_SEP_END - Y_LABEL, C_BLACK);
    lcd_fill(0, H_HEAD, 240, 1, C_LINE);
    lcd_fill(0, Y_FOOT - 2, 240, 1, C_LINE);
    for (i = 1; i < 4u; i++)
        lcd_fill(i * 60u - 1u, H_HEAD + 4, 1, Y_SEP_END - H_HEAD - 4, C_LINE);
}

/* one column: [icon] LABEL / value unit / gauge, redrawn only when it changed.
 * ratio: 0..1000 for the gauge, -1 = no gauge. icon: ICON_* (icons.c), ICON_AUTO = by label */
#define LABEL_X (FELUCCA_ICONS ? ICON_CELL + ICON_GAP : 0)
UI_SMALL static void draw_column(uint32_t c, const char *label, const char *val, const char *unit, uint16_t vc,
                        int32_t ratio, uint32_t icon)
{
    char l[8], v[8], u[8], key[32];
    int32_t x, gw = 52, fx;
    if (icon == ICON_AUTO)
        icon = icon_for_label(label);
    fit(l, label, &FONT_S, 54 - LABEL_X);
    fit(v, val, &FONT_S, is_eng_name(val) ? 56 : 40);   /* engine names whole */
    fit(u, unit, &FONT_S, 54 - text_w(&FONT_S, v) - 3);
    str_cpy(key, l, 8);                                 /* cache key: texts + colour + gauge */
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), v, 8);
    str_cpy(key + str_len(key), "|", 2);
    str_cpy(key + str_len(key), u, 8);
    {
        uint32_t n = str_len(key);
        key[n] = (char)('A' + (vc == C_WHITE) + (vc == C_DIM) * 2);
        key[n + 1] = (char)(' ' + (ratio < 0 ? 0 : 1 + ratio / 20));
        key[n + 2] = (char)(icon == ICON_NONE ? '~' : '!' + icon % 90u);   /* same label, other icon */
        key[n + 3] = 0;
    }
    if (c < 4u) {                                       /* (the big values: their own fit) */
        str_cpy(ui.big_l[c], l, 8);
        fit(ui.big_v[c], val, &FONT_L, 112 - (unit[0] ? 4 : 0));
        str_cpy(ui.big_u[c], unit, 8);
        ui.big_c[c] = vc;
    }
    if (c == ui.hot_col) {
        str_cpy(ui.focus_l, l, 8);
        str_cpy(ui.focus_v, v, 8);
        str_cpy(ui.focus_u, u, 8);
    }
    if (!ui.force && str_eq(key, ui.col[c]))
        return;
    str_cpy(ui.col[c], key, sizeof ui.col[c]);
    if (pal_ribbon) {                                   /* K-GLITCH: a card, its knob's colour on the left edge */
        int32_t ch = Y_SEP_END - Y_LABEL;
        cv_begin(56, (uint32_t)ch, C_BLACK);
        if (l[0] || v[0]) {
            cv_card(0, 0, 56, ch, pal_card, vc == C_WHITE ? C_WHITE : pal_card);
            cv_rect(0, 6, 2, ch - 12, TE_COL[c & 3u]);
            cv_text(6, 3, &FONT_T, l, C_GRAY);
            x = cv_text(6, 15, &FONT_S, v, vc);
            cv_text(x + 3, 20, &FONT_T, u, C_DIM);
            if (ratio >= 0) {
                fx = ratio * 44 / 1000;
                cv_rect(6, ch - 6, 44, 2, pal_row);
                cv_rect(6, ch - 6, fx, 2, vc == C_DIM ? C_DIM : TE_COL[c & 3u]);
                cv_rect(6 + fx, ch - 8, 1, 6, vc == C_DIM ? C_HI : vc);
            }
        }
        cv_blit(c * 60u + 2u, Y_LABEL);
        return;
    }
    cv_begin(55, Y_SEP_END - Y_LABEL, C_BLACK);         /* x 4..58: the rule at 59 stays */
    if (FELUCCA_ICONS && icon != ICON_NONE && l[0])
        cv_icon(0, 1, icon, TE_COL[c & 3u]);            /* icon rows 1..10 = the label's cap height */
    cv_text(l[0] ? LABEL_X : 0, 0, &FONT_S, l, C_GRAY);
    x = cv_text(0, Y_VALUE - Y_LABEL, &FONT_S, v, vc);
    cv_text(x + 3, Y_VALUE - Y_LABEL, &FONT_S, u, C_DIM);
    if (ratio >= 0) {                                   /* gauge: track, fill, 1 px end line */
        int32_t gy = Y_GAUGE - Y_LABEL;
        fx = ratio * gw / 1000;
        cv_rect(0, gy + 1, gw, 1, C_LINE);
        cv_rect(0, gy, fx, 3, vc == C_DIM ? C_DIM : TE_DIM[c & 3u]);
        cv_rect(fx, gy - 1, 1, 5, vc == C_DIM ? C_HI : vc);
    }
    cv_blit(c * 60u + 4u, Y_LABEL);
}

/* Matches voice.c: attack is linear, decay and release are exponential
 * (env += (target - env) * k each tick, ~99 % after the set time). Time
 * axis is the parameter value (the times themselves are exponential). */
UI_SMALL static void graph_adsr(const track_t *t, uint16_t c)
{
    int32_t a = 4 + t->p[P_ATK] * 50 / 127, d = 6 + t->p[P_DEC] * 50 / 127, r = 6 + t->p[P_REL] * 60 / 127;
    int32_t top = 8, bot = 90, sus = t->p[P_SUS] * 1000 / 127;          /* 0..1000 */
    int32_t x0 = 6, x1 = x0 + a, x3 = 232 - r, i, px, py;
    int32_t e = 32768;                                                  /* exp(-4.6 u), Q15 */
#define EGY(lvl) (bot - (lvl) * (bot - top) / 1000)
    cv_line(x0, bot, x1, top, c);                                       /* attack: linear */
    px = x1;
    py = top;
    for (i = 1; i <= d; i++) {                                          /* decay: exponential to SUS */
        int32_t lvl;
        e = (e * (32768 - 150733 / d)) >> 15;           /* k^d = exp(-4.6) */
        lvl = sus + ((1000 - sus) * e >> 15);
        cv_line(px, py, x1 + i, EGY(lvl), c);
        px = x1 + i;
        py = EGY(lvl);
    }
    cv_line(px, py, x3, EGY(sus), c);                                    /* sustain */
    px = x3;
    py = EGY(sus);
    e = 32768;
    for (i = 1; i <= r; i++) {                                          /* release: exponential to 0 */
        e = (e * (32768 - 150733 / r)) >> 15;
        cv_line(px, py, x3 + i, EGY(sus * e >> 15), c);
        px = x3 + i;
        py = EGY(sus * e >> 15);
    }
    cv_line(0, bot + 1, 239, bot + 1, C_LINE);
#undef EGY
}

UI_SMALL static void graph_lfo(const track_t *t, uint16_t c)
{
    int32_t x, py = 50;
    uint32_t ph = (uint32_t)t->p[P_LPHASE] << 25;
    for (x = 0; x < 240; x++) {                      /* (lfo_wave only reads the track) */
        int32_t y = 50 - lfo_wave((track_t *)t, ph + (uint32_t)x * (0xFFFFFFFFu / 120u)) * 38 / 32768;
        if (t->p[P_LWAVE] == 4)
            y = 50 - ((int32_t)((x / 20 * 2654435761u) >> 16) - 32768) * 38 / 32768;
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
    cv_line(0, 50, 239, 50, C_LINE);
}

UI_SMALL static void graph_steps(const track_t *t, uint16_t c)
{
    uint32_t i, len = (uint32_t)t->p[P_SLEN];
    for (i = 0; i < NSTEP; i++) {                    /* 4 rows of 16 thin bars */
        int32_t x = 6 + (int32_t)(i % 16u) * 14 + (int32_t)(i % 16u) / 4 * 4, y = 6 + (int32_t)(i / 16u) * 24;
        const step_t *st = &t->step[i];
        if (i >= len)
            continue;
        if (step_on(st))
            cv_rect(x, y, 1, 14, c);
        else if (st->time == ST_TIE)
            cv_rect(x, y + 6, 1, 8, C_DIM);
        else
            cv_rect(x, y + 13, 1, 1, C_DIM);
        if (st->flags & SF_ACCENT)
            cv_rect(x - 1, y - 2, 3, 1, c);
        if ((song.playing && i == t->seq_idx) || i == ui.cursor)
            cv_rect(x - 1, y + 16, 3, 3, C_WHITE);
    }
}
/* STEP page: the cursor's 16-step bank as a little piano roll */
/* (2.7.2) SEQ > NOTES: the pattern as a piano roll (cv_oy 0, the whole strip). Each note a bar as long as its
 * step and TIEs, its FORCE as its fill (GHOST dark, SOFT hollow, NORM / HARD full, ACC a white cap), a SLIDE's line to
 * the next; the note under edit outlined in white, its name over it; the C rows named on the left, the beats ruled;
 * the playhead a line. The rows follow the note under edit when the pattern spans more than 18 semitones */
UI_SMALL static void graph_roll(const track_t *t, uint16_t c)
{
    uint32_t len = trk_len(t), nv = NV_ZOOM[clamp(1 + (int8_t)ui.zoom, 0, 3)], v0, i, j;
    int32_t lo = 127, hi = 0, x0 = 22, w = 240 - 22 - 2, top = 14, bot = H_GRAPH - 4 - (ui.sheet ? 18 : 0), rh, sel_note = last_note, cx;
    const step_t *cs = &t->step[ui.cursor];
    char b[12];
    if (nv > len)
        nv = len;
    v0 = ui.cursor / nv * nv;
    for (i = 0; i < len; i++)
        for (j = 0; j < t->step[i].n; j++) {
            if (t->step[i].note[j] < lo) lo = t->step[i].note[j];
            if (t->step[i].note[j] > hi) hi = t->step[i].note[j];
        }
    if (step_on(cs))
        sel_note = cs->note[ui.note_k % cs->n];
    if (lo > hi)
        lo = hi = sel_note;
    if (hi - lo < 11) {                                 /* (an octave at least: rows of a sane height) */
        lo = clamp((lo + hi) / 2 - 6, 0, 115);
        hi = lo + 11;
    }
    if (hi - lo > 17) {                                 /* (a wide pattern: 18 rows around the note under edit) */
        lo = clamp(sel_note - 8, lo, hi - 17);
        hi = lo + 17;
    }
    lo = clamp(lo - 1, 0, 127), hi = clamp(hi + 1, 0, 127);
    rh = (bot - top) / (hi - lo + 1);
    fmt_int(b, (int32_t)v0 + 1);
    str_cpy(b + str_len(b), "-", 2);
    fmt_int(b + str_len(b), (int32_t)(v0 + nv > len ? len : v0 + nv));
    cv_text(2, 0, &FONT_S, b, C_GRAY);
    str_cpy(b, "ZOOM ", 12);
    fmt_int(b + 5, (int32_t)nv);
    cv_text(238 - text_w(&FONT_S, b), 0, &FONT_S, b, C_DIM);
    for (i = (uint32_t)lo; i <= (uint32_t)hi; i++) {    /* rows: the black keys' bands, the C's ruled and named */
        int32_t y = bot - ((int32_t)i - lo + 1) * rh;
        uint32_t pc = i % 12u;
        if ((0x54Au >> pc) & 1u)
            cv_rect(x0, y, w, rh, TE_G1);
        if (!pc) {
            note_name(b, i);
            cv_text(0, y + rh / 2 - 6, &FONT_S, b, C_DIM);
            cv_rect(x0, y + rh - 1, w, 1, C_GRAY);
        }
    }
    for (i = 0; i <= nv; i++)                           /* the beats (4 steps), a tick on each step */
        if (i % 4u == 0u)
            cv_rect(x0 + (int32_t)i * w / (int32_t)nv, top, 1, bot - top, i % 16u ? C_DIM : C_GRAY);
        else if (nv <= 32u)
            cv_rect(x0 + (int32_t)i * w / (int32_t)nv, bot + 1, 1, 2, C_DIM);
    for (i = 0; i < len; i++) {                         /* the notes (one starting before the view: its part in it) */
        const step_t *st = &t->step[i];
        uint32_t n = note_len(t, i), f;
        int32_t xa, xb, y;
        if (!step_on(st) || i + n <= v0 || i >= v0 + nv)
            continue;
        xa = x0 + ((int32_t)i - (int32_t)v0) * w / (int32_t)nv + 1;
        xb = x0 + ((int32_t)(i + n) - (int32_t)v0) * w / (int32_t)nv - 1;
        xa = xa < x0 ? x0 : xa;
        xb = xb > x0 + w ? x0 + w : xb;
        for (j = 0; j < st->n; j++) {
            if (st->note[j] < lo || st->note[j] > hi)
                continue;
            y = bot - ((int32_t)st->note[j] - lo + 1) * rh + 1;
            f = note_force(st, j);
            cv_rect(xa, y, xb - xa, rh - 1, f ? c : TE_DIM[song.sel & 3u]);
            if (f == 1u && rh > 4)                      /* SOFT: hollow */
                cv_rect(xa + 1, y + 1, xb - xa - 2, rh - 3, C_BLACK);
            if (f == 4u)                                /* ACC: a white cap */
                cv_rect(xa, y, xb - xa, 2, C_WHITE);
            if ((st->flags & SF_SLIDE) && j == 0u)
                cv_rect(xb, y + rh / 2 - 1, 4, 2, c);
            if (i == ui.cursor && j == ui.note_k % st->n) {   /* the note under edit */
                cv_rect(xa - 2, y - 2, xb - xa + 4, 1, C_WHITE), cv_rect(xa - 2, y + rh - 1, xb - xa + 4, 1, C_WHITE);
                cv_rect(xa - 2, y - 2, 1, rh + 2, C_WHITE), cv_rect(xb + 1, y - 2, 1, rh + 2, C_WHITE);
                note_name(b, st->note[j]);
                cv_text(xa, y - 15 < top ? y + rh + 1 : y - 15, &FONT_S, b, C_WHITE);
            }
        }
    }
    cx = x0 + ((int32_t)ui.cursor - (int32_t)v0) * w / (int32_t)nv;
    if (!step_on(cs) && sel_note >= lo && sel_note <= hi) {   /* an empty step: a frame where a note would go */
        int32_t y = bot - (sel_note - lo + 1) * rh + 1, xw = w / (int32_t)nv - 2;
        cv_rect(cx + 1, y - 1, xw, 1, C_WHITE), cv_rect(cx + 1, y + rh - 1, xw, 1, C_WHITE);
        cv_rect(cx + 1, y - 1, 1, rh, C_WHITE), cv_rect(cx + xw, y - 1, 1, rh, C_WHITE);
    }
    if (song.playing && t->seq_idx >= v0 && t->seq_idx < v0 + nv) {   /* the playhead */
        int32_t x = x0 + ((int32_t)t->seq_idx - (int32_t)v0) * w / (int32_t)nv;
        cv_rect(x, top, 1, bot - top, C_HI);
        cv_rect(x - 2, top, 5, 2, C_HI);
    }
    if (ui.sheet) {                                     /* OCT+ held: the note's sheet */
        char nu[8];
        const char *val[6] = {cs->flags & SF_SLIDE ? "ON" : "OFF", nu, FILL_N[step_fill(t, ui.cursor) % 3u], "", "", ""};
        fmt_int(nu, t->micro[ui.cursor]);
        if (t->micro[ui.cursor] > 0) { nu[0] = '+'; fmt_int(nu + 1, t->micro[ui.cursor]); }
        sheet_bar(H_GRAPH - 16, SHEET_N, val, ui.sheet - 1u, TE_DIM[song.sel & 3u]);
    }
}
UI_SMALL static void graph_scale(const track_t *t, uint16_t c)
{
    static const uint8_t BLACK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    uint32_t i, mask = scale_mask(t);
    for (i = 0; i < 12u; i++) {
        uint32_t deg = (i + 12u - (uint32_t)t->p[P_ROOT]) % 12u;
        int32_t x = 6 + (int32_t)i * 19;
        uint16_t col = (mask >> deg) & 1u ? (deg == 0u ? C_WHITE : c) : C_DIM;
        cv_rect(x, BLACK[i] ? 10 : 40, 16, 1, col);
        cv_rect(x + 7, BLACK[i] ? 10 : 40, 1, 40, col);
    }
}

UI_SMALL static void graph_fx(const track_t *t, uint16_t c)
{
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        int32_t h = t->p[P_DIST + i] * 80 / 127, x = (int32_t)i * 60 + 28;
        cv_rect(x, 10, 1, 80, C_LINE);
        cv_rect(x, 90 - h, 1, h, c);
        cv_rect(x - 3, 90 - h, 7, 1, c);
    }
}
/* SLICER page: the pattern's 16 steps, a 'x' step a full bar; a '.' step: GATE a bar as high as
 * it stays open (DEPTH), STUT hatched (it repeats the last 'x'); the step playing underlined.
 * Grey when the SLICER is OFF. */
/* K-GLITCH: EDIT > DJ FILTER: the track's filter as a curve (as fx.c djf_block sets it: the square-root law, 12 dB an
 * octave, the resonance's peak), its cutoff marked and named in Hz; open (0): a flat line */
UI_SMALL static void graph_tflt(const track_t *t, uint16_t c)
{
    static const uint8_t SQ[65] = {0, 14, 20, 24, 28, 31, 34, 37, 39, 42, 44, 46, 48, 50, 52, 54, 56, 57, 59, 60, 62, 64,
                                   65, 67, 68, 69, 71, 72, 73, 75, 76, 77, 78, 80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 91,
                                   92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 107, 108, 109,
                                   110, 111};
    int32_t v = t->p[P_TFLT], a = SQ[v < 0 ? (-v > 64 ? 64 : -v) : (v > 64 ? 64 : v)], cut = v < 0 ? 127 - a : a, x, py = 0;
    int32_t pk = song.g[G_KTR1 + song.sel % 4u] * 12;   /* the peak, dB x 10 */
    char b[16];
    cv_line(8, 36, 232, 36, C_LINE);                    /* 0 dB */
    for (x = 8; x <= 232; x++) {
        int32_t i = (x - 8) * 127 / 224, d = v < 0 ? i - cut : cut - i, db = 0, y;   /* (dB x 10) */
        if (v && d > 0) db = -d * 86 / 10;              /* 12 dB an octave: 0.86 dB a step */
        if (v && d > -8 && d < 8) db += pk * (64 - d * d) / 64;   /* (the resonance: a rounded peak) */
        y = clamp(36 - db / 5, 4, 96);
        if (x > 8) cv_line(x - 1, py, x, y, c);
        py = y;
    }
    if (v) {
        uint32_t hz = CUTOFF_HZ[clamp(cut, 0, 127)];
        int32_t cx = 8 + cut * 224 / 127;
        cv_line(cx, 8, cx, 96, C_WHITE);
        str_cpy(b, v < 0 ? "LP " : "HP ", sizeof b);
        fmt_int(b + 3, (int32_t)(hz < 1000u ? hz : hz / 100u));
        if (hz >= 1000u) {                             /* "2.4K" */
            uint32_t n = str_len(b);
            b[n] = b[n - 1u], b[n - 1u] = '.', b[n + 1u] = 'K', b[n + 2u] = 0;
        } else
            str_cpy(b + str_len(b), " HZ", sizeof b - str_len(b));
        cv_text(cx > 150 ? cx - 6 - text_w(&FONT_S, b) : cx + 6, 8, &FONT_S, b, C_WHITE);
    } else {
        cv_text(80, 8, &FONT_S, "FILTER OPEN", C_DIM);
    }
}
UI_SMALL static void graph_slicer(const track_t *t, uint16_t c)
{
    uint32_t i, pat = sl_pattern(t), mode = (uint32_t)t->p[P_SLCR], cur = sl[t - trk].idx;
    int32_t open = 70 - t->p[P_SLDEPTH] * 70 / 127;      /* px a closed GATE step keeps */
    uint16_t col = mode == SL_OFF ? C_DIM : c;
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + (int32_t)i * 14 + (int32_t)(i / 4u) * 2, y;
        if ((pat >> i) & 1u) {
            cv_rect(x, 10, 11, 70, col);
        } else if (mode == SL_STUT) {
            for (y = 10; y < 80; y += 4)
                cv_rect(x, y, 11, 1, col);
        } else {
            cv_rect(x, 79, 11, 1, C_LINE);
            if (open)
                cv_rect(x, 80 - open, 11, open, C_DIM);
        }
        if (mode != SL_OFF && i == cur)
            cv_rect(x, 85, 11, 3, C_WHITE);
    }
}
UI_SMALL static uint32_t steps_hash(const track_t *t)
{
    uint32_t h = 2166136261u, i;
    for (i = 0; i < NSTEP; i++) {
        const step_t *st = &t->step[i];
        h = (h ^ (st->note[0] + st->n * 128u + st->time * 1024u + st->flags * 4096u + st->note[1] * 65536u)) *
            16777619u;
    }
    return h;
}

UI_SMALL static uint32_t str_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

/* a page drawn with its values large (graph_big) */
UI_SMALL static int big_page(const page_t *pg)            /* (2.6: the INSERT pages too: their middle stayed empty) */
{
    return (pg->graph == GR_NONE || pg->graph == GR_INS) && pg->scope != SC_SONG && pg->scope != SC_DRUM;
}
UI_SMALL static uint32_t graph_signature(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint32_t h = 2166136261u, i;
    if (ui.hot_t && settings.zoom)
        h = str_hash(str_hash(str_hash(h ^ 0x5555u, ui.focus_v), ui.focus_l), ui.focus_u);
    if (!ui.home && big_page(pg))                    /* the big values: as the columns show them */
        for (i = 0; i < 4u; i++)
            h = str_hash(str_hash(str_hash(h ^ ui.big_c[i] * 31u, ui.big_v[i]), ui.big_l[i]), ui.big_u[i]);
    if (ui.home)
        return h ^ (ui.frame / 2u);                  /* scope: redraw every other frame */
    h ^= (uint32_t)pg->graph * 131u + TSEL->eng_req + song.sel * 7777u;
    for (i = 0; i < P_COUNT; i++)
        h = (h ^ (uint32_t)t->p[i]) * 16777619u;
    h ^= (uint32_t)TSEL->preset * 7u + (uint32_t)song.g[G_SLOT] * 13u + TSEL->user * 257u + up_gen * 7919u + ui.uslot * 104729u;
    {   /* (2.7) the favourites: the PRESETS list and its FAV column */
        uint32_t q;
        for (q = 0; q < FAV_N; q++) h = h * 31u + fav[q];
    }
    if (pg->graph == GR_TFLT)                          /* (DJ FILTER: its resonance) */
        h ^= (uint32_t)song.g[G_KTR1 + song.sel % 4u] * 2654435761u;
    if (pg->graph == GR_SLCR && t->p[P_SLCR])        /* the SLICER's step playing */
        h ^= (sl[song.sel].idx + 1u) * 2654435761u;
    if (pg->graph == GR_SLOTS)                       /* (a checksum over each slot) */
        for (i = 0; i < NSONG; i++)
            h = h * 31u + (uint32_t)project_used(i) + (uint32_t)(uint8_t)song_name_of(i)[0] * 3u;
    if (pg->graph == GR_NAME)                        /* (the name, the cursor) */
        for (i = 0; i <= KGS_NAME; i++)
            h = h * 31u + (uint8_t)song_name[i] + name_pos * 7u;
    if (pg->graph == GR_STEPS || pg->graph == GR_ROLL) {
        uint32_t ph = song.playing ? t->seq_idx : 0xFFFFu;
        if (pg->graph == GR_ROLL)                    /* (2.7.2 NOTES: the note under edit, the zoom, the sheet) */
            h ^= ui.note_k * 104729u + ui.zoom * 7u + ui.sheet * 977u + (uint32_t)t->micro[ui.cursor] * 31337u +
                 step_fill(t, ui.cursor) * 3u + last_note * 65537u;
        h ^= steps_hash(t) + ph * 31u + ui.cursor * 7919u;
    }
    return h;
}
/* preset browser: the list by kind (ui.c BANK), the current one in white */
UI_SMALL static void graph_browse(void)
{
    uint32_t total, cur = preset_pos(&total), e, k;
    int32_t row;
    if (!total)
        return;
    for (row = -3; row <= 3; row++) {
        int32_t y = 4 + (row + 3) * 17;
        char tag[5], nm[13];
        int sel = row == 0;
        uint32_t n = (cur + total * 4u + (uint32_t)row) % total;
        e = preset_at(n, &k);
        if (e == NENGINES) {                             /* user preset: "U07" and its name */
            up_slot_label(tag, k);
            up_name(k, nm);
        } else {                                         /* its kind: BASS, KEYS, PAD... */
            str_cpy(tag, preset_kind(n), sizeof tag);
            str_cpy(nm, ENGINES[e]->presets[k].name, sizeof nm);
        }
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_GRAY : C_DIM);
        cv_text(54, y, &FONT_S, nm, sel ? C_WHITE : C_GRAY);
    }
}

/* user preset slots around the selected one: "U07  NAME" / EMPTY, the selected one in white */
UI_SMALL static void graph_user(void)
{
    int32_t row, first = clamp((int32_t)ui.uslot - 3, 0, UP_SLOTS - 7);
    for (row = 0; row < 7; row++) {
        uint32_t k = (uint32_t)(first + row);
        int32_t y = 4 + row * 17;
        char tag[4], nm[13];
        int sel = k == ui.uslot, used = up_used(k);
        up_slot_label(tag, k);
        if (used)
            up_name(k, nm);
        else
            str_cpy(nm, "EMPTY", sizeof nm);
        if (sel)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, tag, sel ? C_WHITE : C_GRAY);
        cv_text(54, y, &FONT_S, nm, used ? (sel ? C_WHITE : C_HI) : C_DIM);
    }
}

/* K-GLITCH: the 16 songs (A..D: the song mode's sections), five rows around the selected one: its letter, its name
 * (or SONG), used / empty; the selected one in white */
UI_SMALL static void graph_slots(void)
{
    int32_t sel = (int32_t)song.g[G_SLOT] - 1, first = clamp(sel - 2, 0, (int32_t)NSONG - 5), row;
    for (row = 0; row < 5; row++) {
        uint32_t k = (uint32_t)(first + row);
        int32_t y = 4 + row * 22;
        int on = (int32_t)k == sel, used = project_used(k);
        const char *nm = song_name_of(k);
        char b[4];
        b[0] = (char)('A' + k);
        b[1] = 0;
        if (on)
            cv_rect(4, y + 6, 3, 3, C_WHITE);
        cv_text(14, y, &FONT_S, b, on ? C_WHITE : k < 4u ? C_HI : C_GRAY);
        cv_text(40, y, &FONT_S, used ? (nm[0] ? nm : "SONG") : "EMPTY", used ? (on ? C_WHITE : C_HI) : C_DIM);
    }
}

/* K-GLITCH SAVE > NAME: the name with its cursor, and the keyboard the keys are (after K-WASUB): the 16 white keys
 * L .. Z and a space, the 11 black ones A .. K; K2 a digit or a sign at the cursor */
UI_SMALL static void graph_name(void)
{
    static const char WH[16] = {'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_'};
    const int32_t kx = 8, ky = 52, kw = 14, kh = 54;
    int32_t i, x;
    char t[2] = {0, 0}, b[16];
    cv_text(8, 2, &FONT_S, "SONG", C_GRAY);
    t[0] = (char)('A' + (uint32_t)(song.g[G_SLOT] - 1) % NSONG);
    cv_text(52, 2, &FONT_S, t, C_HI);
    for (i = 0, x = 8; i < (int32_t)KGS_NAME; i++) {   /* the name, a box a character, the cursor's box lit */
        int on = i == (int32_t)name_pos;
        t[0] = i < (int32_t)str_len(song_name) ? song_name[i] : ' ';
        cv_rect(x, 22, 17, 22, on ? C_WHITE : C_DIM);
        cv_rect(x + 1, 23, 15, 20, on ? C_AMB : C_BLACK);
        cv_text(x + 4, 24, &FONT_S, t, on ? C_BLACK : C_WHITE);
        x += 19;
    }
    (void)b;
    for (i = 0; i < 16; i++) {                       /* the white keys */
        cv_rect(kx + i * kw, ky, kw - 1, kh, C_HI);
        t[0] = WH[i];
        cv_text(kx + i * kw + 3, ky + kh - 16, &FONT_S, t, C_BLACK);
    }
    for (i = 0; i < 11; i++) {                       /* the black ones, between the white keys before and after them */
        int32_t w = punch_key(PUNCH_BLACK[i] - 1u), bx = kx + (w + 1) * kw - 5;
        cv_rect(bx, ky, 11, kh * 55 / 100, C_BLACK);
        t[0] = (char)('A' + i);
        cv_text(bx + 2, ky + 4, &FONT_S, t, C_AMB);
    }
}

/* TRACKS page: four channel strips (mixer style), one under each column: number +
 * REC / ARM / MUTE, the sound's short name, then a level fader with the output meter
 * (note activity), the pattern over its LEN (time running down; bar width = notes in
 * the step) and the play head. The selected track is drawn bright. Every part is
 * its own small canvas with its own signature: while the transport runs only the
 * head markers and the meters move. (No ZOOM focus here.) */
#define TS_HEAD_Y (Y_GRAPH + 2)
#define TS_NAME_Y (Y_GRAPH + 20)
#define TS_Y (Y_GRAPH + 38)
#define TS_H 84                                      /* body: rows Y_GRAPH + 38 .. + 121 */
static struct {
    uint32_t head[NTRK], name[NTRK], fader[NTRK], ov[NTRK], mark[NTRK];
    uint8_t meter[NTRK];
} ts;

UI_SMALL static int32_t meter_px(int32_t a)                   /* |sample| (Q15) -> px: 6 dB = TS_H / 10 */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);             /* 8 log2(a): 48 (-54 dB) .. 128 (+6 dB) */
    return clamp((v - 48) * (TS_H - 2) / 80, 0, TS_H - 2);
}

UI_SMALL static uint32_t trk_level(uint32_t c)                /* LEVEL 0..127 (the drum track: GLO > DRUMS LEVEL) */
{
    return (uint32_t)(c == TRK_DRUM ? song.g[G_DRLVL] : trk[c].p[P_LEVEL]) & 127u;
}

UI_SMALL static void trk_short_name(uint32_t c, char *b)      /* the track's sound, b holds 13 */
{
    const track_t *t = &trk[c];
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    if (c == TRK_DRUM)
        str_cpy(b, "DRUM", 13);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), b);
    else if (e->npresets)
        str_cpy(b, e->presets[t->preset % e->npresets].name, 13);
    else
        str_cpy(b, e->name, 13);
}

UI_SMALL static void draw_tracks(void)
{
    uint32_t c;
    if (ui.force) {
        lcd_fill(0, Y_GRAPH, 240, H_GRAPH, C_BLACK);
        for (c = 0; c < NTRK; c++)
            ts.meter[c] = 0;
    }
    for (c = 0; c < NTRK; c++) {
        track_t *t = &trk[c];
        uint32_t x0 = c * 60u + 4u, sel = c == song.sel, lvl = trk_level(c), mute = !lvl || t->p[P_MUTE];
        uint32_t arm = (song.rec >> c) & 1u, st = arm ? (song.playing ? 1u : 2u) : mute ? 3u : 0u, sig;
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u, row = 0xFFFFu;
        int32_t pk, m;
        char b[16];
        if (c == TRK_DRUM) {
            pk = drums.peak;
            drums.peak = 0;
        } else {
            pk = t->peak;
            t->peak = 0;
        }
        /* head: number (white = selected), REC / ARM / MUTE */
        sig = 1u + sel + st * 2u;
        if (ui.force || sig != ts.head[c]) {
            ts.head[c] = sig;
            b[0] = (char)('1' + c);
            b[1] = 0;
            cv_begin(54, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_WHITE : C_GRAY);
            if (sel)
                cv_rect(0, 15, 8, 1, C_WHITE);
            if (st == 1u || st == 2u) {
                cv_rect(14, 5, 6, 6, st == 1u ? C_WHITE : C_AMB);
                cv_text(24, 0, &FONT_S, st == 1u ? "REC" : "ARM", st == 1u ? C_WHITE : C_AMB);
            } else if (st) {
                cv_text(14, 0, &FONT_S, "MUTE", C_DIM);
            }
            cv_blit(x0, TS_HEAD_Y);
        }
        /* the sound: preset name (user preset, DRUM), cut to the column */
        trk_short_name(c, b);
        while (b[0] && text_w(&FONT_S, b) > 54)
            b[str_len(b) - 1u] = 0;
        sig = str_hash(2u + sel, b);
        if (ui.force || sig != ts.name[c]) {
            ts.name[c] = sig;
            cv_begin(54, 16, C_BLACK);
            cv_text(0, 0, &FONT_S, b, sel ? C_HI : C_DIM);
            cv_blit(x0, TS_NAME_Y);
        }
        /* fader (LEVEL) + meter of the output */
        m = mute ? 0 : meter_px(pk);
        if (m < ts.meter[c] - 3)
            m = ts.meter[c] - 3;                     /* falls ~20 dB/s */
        ts.meter[c] = (uint8_t)(m < 0 ? 0 : m);
        sig = 1u + lvl + ts.meter[c] * 128u + sel * 65536u + (sel && ui.hot_t && ui.hot_col == 1u) * 131072u +
              (t->p[P_MUTE] != 0) * 262144u;
        if (ui.force || sig != ts.fader[c]) {
            int32_t fy = (int32_t)(TS_H - 2u) - (int32_t)lvl * (TS_H - 4) / 127;
            ts.fader[c] = sig;
            cv_begin(12, TS_H, C_BLACK);
            cv_rect(3, 0, 1, TS_H, C_LINE);
            if (lvl) {
                cv_rect(2, fy, 3, TS_H - fy, C_DIM);
                cv_rect(0, fy - 1, 7, 2, (sig & 131072u) ? C_WHITE : sel ? C_HI : C_GRAY);
            } else {
                cv_rect(0, TS_H - 2, 7, 2, C_DIM);
            }
            if (ts.meter[c])
                cv_rect(9, TS_H - ts.meter[c], 3, ts.meter[c], C_AMB);
            cv_blit(x0, TS_Y);
        }
        /* the pattern: one band per step over LEN, a note step as wide as its notes */
        sig = steps_hash(t) * 31u + len * 7u + sel;
        if (ui.force || sig != ts.ov[c]) {
            uint32_t i;
            ts.ov[c] = sig;
            cv_begin(34, TS_H, C_BLACK);
            cv_rect(17, 0, 1, TS_H, C_LINE);
            for (i = 0; i < len; i++) {
                const step_t *s = &t->step[i];
                int32_t r0 = (int32_t)(i * TS_H / len), r1 = (int32_t)((i + 1u) * TS_H / len), h = r1 - r0;
                if (h > 2)
                    h--;                             /* a gap between steps when there is room */
                if (h < 1)
                    h = 1;
                if (step_on(s)) {
                    int32_t w = 2 + 6 * (int32_t)s->n;
                    cv_rect(17 - w / 2, r0, w, h, (s->flags & SF_ACCENT) && sel ? C_WHITE : sel ? C_HI : C_GRAY);
                } else if (s->time == ST_TIE) {
                    cv_rect(16, r0, 3, r1 - r0, C_DIM);
                }
            }
            cv_blit(x0 + 14u, TS_Y);
        }
        /* play head: a small arrow right of the pattern (white while recording) */
        if (song.playing)
            row = ((t->seq_idx % len) * 2u + 1u) * TS_H / (2u * len);
        sig = 1u + row + (st == 1u) * 65536u;
        if (ui.force || sig != ts.mark[c]) {
            int32_t i;
            ts.mark[c] = sig;
            cv_begin(5, TS_H, C_BLACK);
            if (row != 0xFFFFu)
                for (i = -2; i <= 2; i++) {
                    int32_t w = 5 - 2 * (i < 0 ? -i : i);
                    cv_rect(5 - w, (int32_t)row + i, w, 1, st == 1u ? C_WHITE : C_AMB);
                }
            cv_blit(x0 + 49u, TS_Y);
        }
    }
}

/* oscilloscope of the output, triggered on a rising zero crossing */
#include "scratch.h"                                /* sloop-lab: the snapshot in the main loop's shared scratch */
_Static_assert(sizeof main_scratch.snap == SCOPE_N * sizeof(int16_t), "the scratch holds the snapshot");
UI_SMALL static void graph_scope(uint16_t c)
{
    int16_t *snap = main_scratch.snap;
    uint32_t w = scope_w, i, trig = 0;
    int32_t py = 50, x, peak = 1500;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    cv_line(0, 50, 239, 50, C_LINE);
    for (x = 0; x < 240; x++) {
        int32_t y = 50 - snap[trig + (uint32_t)x] * 44 / peak;   /* auto-scaled */
        if (x)
            cv_line(x - 1, py, x, y, c);
        py = y;
    }
}

/* SLOOP 2.4: a page without a graph (EDIT, VOICE, the DEST pages, GLOBAL, MASTER, SYSTEM...): its values
 * large in the empty space, 2 x 2 as the knobs (KNOB 1 2 / KNOB 3 4), the one turned in white; one value
 * alone (FILTER) across the width. The columns (draw_columns, drawn first) fill ui.big_* */
UI_SMALL static void graph_big(void)
{
    uint32_t c, n = 0;
    for (c = 0; c < 4u; c++)
        n += ui.big_l[c][0] != 0;
    if (pal_ribbon) {                                   /* K-GLITCH: the panel's four quarters */
        for (c = 0; c < 4u; c++)
            if (ui.big_l[c][0] && n > 1u)
                cv_card(c & 1u ? 122 : 6, c & 2u ? 63 : 3, 112, 57, pal_card, ui.big_c[c] == C_WHITE ? C_WHITE : pal_card);
    } else if (n > 1u) {
        cv_rect(119, 6, 1, 112, C_LINE);
        cv_rect(6, 62, 228, 1, C_LINE);
    }
    for (c = 0; c < 4u; c++) {
        int32_t x0 = c & 1u ? 126 : 6, y0 = c & 2u ? 64 : 2, x;
        if (!ui.big_l[c][0])
            continue;
        if (n == 1u) {                                  /* one value: in the middle */
            x0 = (240 - text_w(&FONT_L, ui.big_v[c]) - (ui.big_u[c][0] ? text_w(&FONT_S, ui.big_u[c]) + 4 : 0)) / 2;
            y0 = 30;
        }
        if (pal_ribbon) x0 += 4;
        cv_text(x0, y0 + 2, &FONT_S, ui.big_l[c], pal_ribbon ? C_GRAY : TE_COL[c & 3u]);
        x = cv_text(x0, y0 + 20, &FONT_L, ui.big_v[c], ui.big_c[c] == C_DIM ? C_DIM : ui.big_c[c] == C_WHITE ? C_WHITE : C_HI);
        if (ui.big_u[c][0])
            cv_text(x + 4, y0 + 34, &FONT_S, ui.big_u[c], C_GRAY);
    }
}

UI_SMALL static void draw_graph(void)
{
    const page_t *pg = cur_page();
    const track_t *t = TSEL;
    uint16_t c = TE_COL[song.sel & 3u];               /* LIVE: the curves in the track's colour */
    uint32_t sig, top, drum_note = !ui.home && is_drum(t) && !page_for_drum(pg);
    if (!ui.home && pg->graph == GR_TRK) {
        draw_tracks();
        ui.graph_top = 1;                            /* the next graph draws its top rows again */
        return;
    }
    sig = graph_signature();
    if (!ui.force && sig == ui.graph_sig)
        return;
    ui.graph_sig = sig;
    {
    uint16_t bg0 = pal_bg;
    if (pal_ribbon)                                     /* K-GLITCH: the graph on a panel (its background drawn as "black") */
        pal_bg = pal_row;
    cv_begin(240, H_GRAPH, C_BLACK);
    cv_oy = G_OY;
    if (ui.home) {
        graph_scope(c);
    } else if (drum_note) {                          /* a page the drum track has no use for */
        static const char *const L[2] = {"DRUM TRACK", "SEQ  TRACKS  GLO DRUMS"};
        cv_text((240 - text_w(&FONT_S, L[0])) / 2, 26, &FONT_S, L[0], C_HI);
        cv_text((240 - text_w(&FONT_S, L[1])) / 2, 52, &FONT_S, L[1], C_DIM);
    } else {
        switch (pg->graph) {
        case GR_ADSR:
            graph_adsr(t, c);
            break;
        case GR_LFO:
            graph_lfo(t, c);
            break;
        case GR_STEPS:
            graph_steps(t, c);
            break;
        case GR_ROLL:
            cv_oy = 0;
            graph_roll(t, c);
            break;
        case GR_SCALE:
            graph_scale(t, c);
            break;
        case GR_FX:
            graph_fx(t, c);
            break;
        case GR_SLCR:
            graph_slicer(t, c);
            break;
        case GR_TFLT:
            graph_tflt(t, c);
            break;
        case GR_BROWSE:
            cv_oy = 0;
            graph_browse();
            break;
        case GR_SLOTS:
            cv_oy = 0;
            graph_slots();
            break;
        case GR_NAME:
            cv_oy = 0;
            graph_name();
            break;
        case GR_USER:
            cv_oy = 0;
            graph_user();
            break;
        default:
            if (big_page(pg)) {
                cv_oy = 0;
                graph_big();
            }
            break;
        }
    }
    top = !ui.home && !drum_note && (pg->graph == GR_BROWSE || pg->graph == GR_SLOTS || pg->graph == GR_NAME || pg->graph == GR_USER ||
                                     pg->graph == GR_ROLL || big_page(pg));   /* these draw from the top */
    cv_oy = 0;
    if (ui.hot_t && settings.zoom && !(top && (big_page(pg) || pg->graph == GR_ROLL) && !drum_note)) {   /* (the big values show it already) */                 /* focus (menu ZOOM): the touched value, large and white */
        int32_t x;
        top = 1;
        cv_rect(0, 0, 150, 50, C_BLACK);
        cv_text(4, 0, &FONT_S, ui.focus_l, C_GRAY);
        x = cv_text(4, 16, &FONT_L, ui.focus_v, C_WHITE);
        cv_text(x + 4, 30, &FONT_S, ui.focus_u, C_DIM);
    }
    if (pal_ribbon) {                                   /* the panel's margins and rounded corners, the page's background */
        pal_bg = bg0;
        cv_rect(0, 0, 3, H_GRAPH, bg0), cv_rect(237, 0, 3, H_GRAPH, bg0);
        cv_rect(3, 0, 2, 1, bg0), cv_rect(3, 1, 1, 1, bg0), cv_rect(235, 0, 2, 1, bg0), cv_rect(236, 1, 1, 1, bg0);
        cv_rect(3, H_GRAPH - 1, 2, 1, bg0), cv_rect(3, H_GRAPH - 2, 1, 1, bg0);
        cv_rect(235, H_GRAPH - 1, 2, 1, bg0), cv_rect(236, H_GRAPH - 2, 1, 1, bg0);
        top = 1;                                        /* (the whole panel each time: its margins) */
    }
    pal_bg = bg0;
    /* graphs keep out of the top G_OY rows: skip them unless something is (or was) there */
    cv_blit_from(0, Y_GRAPH, top || ui.graph_top || ui.force ? 0u : G_OY);
    ui.graph_top = (uint8_t)top;
    }
}

/* the page shown: its place in its family and how many (*n), as SELECT walks them (2.7: the screens of their own, SONG
 * and DRUMS, are not counted: SEQ is STEP 1/2, PATTERN 2/2; the page shown always counts) */
UI_SMALL static uint32_t page_pos(uint32_t *n)
{
    const page_t *pg = cur_page();
    uint32_t i, k = 0;
    *n = 0;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == pg->fam && (i == ui.page || (page_shown(&PAGES[i]) && PAGES[i].scope != SC_SONG &&
                                                         PAGES[i].scope != SC_DRUM))) {
            (*n)++;
            if (i == ui.page)
                k = *n;
        }
    return k;
}

UI_SMALL static void draw_foot(void)
{
    char s[48], pn[16], en[10], ti[20];
    const track_t *t = TSEL;
    uint32_t sig;
    const page_t *pg = cur_page();
    const engine_t *e = ENGINES[TSEL->eng_req % NENGINES];
    const char *ename = is_drum(t) ? "DRUM" : e->name;
    int32_t x;
    pn[0] = 0;
    if (is_drum(t))
        str_cpy(pn, DRUM_KIT_NAMES[drum_kit()], sizeof pn);
    else if (user_of(t) < UP_SLOTS)
        up_name(user_of(t), pn);                       /* a user preset */
    else if (e->npresets)
        str_cpy(pn, e->presets[TSEL->preset % e->npresets].name, sizeof pn);
    if (ui.home) {
        str_cpy(ti, "HOME", sizeof ti);
    } else {                                           /* page title + number in its family: "ENV DEST 2/2" */
        uint32_t n, k = page_pos(&n);
        const char *pt = pg->scope == SC_ENGINE ? e->page_title[pg->id[0] != P_E0] : 0;   /* EDIT: the engine's */
        str_cpy(ti, pt ? pt : pg->title, 10);
        if (n > 1) {
            str_cpy(ti + str_len(ti), " ", 4);
            fmt_int(ti + str_len(ti), (int32_t)k);
            str_cpy(ti + str_len(ti), "/", 4);
            fmt_int(ti + str_len(ti), (int32_t)n);
        }
    }
    str_cpy(s, ename, sizeof s);
    s[str_len(s) + 1u] = 0;
    s[str_len(s)] = (char)('1' + song.sel);
    str_cpy(s + str_len(s), pn, 16);
    str_cpy(s + str_len(s), ti, sizeof ti);
    {   /* step markers: the playhead only when it is in the shown bank, the cursor only in SEQ */
        uint32_t ph = song.playing && t->seq_idx / 16u == ui.bank ? t->seq_idx : 0xFFu;
        sig = str_hash(0x9E3779B9u, s) + ph * 97u + (song.seq_mode ? ui.cursor : 0xFFu) * 3001u + steps_hash(t) +
              ui.bank * 7u + (uint32_t)t->p[P_SLEN] * 13u;
    }
    if (!ui.force && sig == ui.foot_sig)
        return;
    ui.foot_sig = sig;
    cv_begin(240, H_FOOT, C_BLACK);
    {
        uint32_t i;
        for (i = 0; i < 16u; i++) {                   /* row 1: the cursor's bank as 16 thin bars */
            uint32_t si = ui.bank * 16u + i;
            int32_t sx = 6 + (int32_t)i * 14 + (int32_t)(i / 4u) * 4;
            const step_t *st = &t->step[si];
            if (si >= (uint32_t)t->p[P_SLEN])
                continue;
            if (step_on(st))
                cv_rect(sx, 2, 2, 9, TE_COL[song.sel & 3u]);
            else
                cv_rect(sx, 10, 1, 1, C_DIM);
            if ((song.playing && si == t->seq_idx) || (song.seq_mode && si == ui.cursor))
                cv_rect(sx - 1, 13, 3, 3, C_WHITE);
        }
    }
    x = 4;
    if (FELUCCA_ICONS) {                              /* row 2: engine icon + name, preset, page */
        cv_icon(x, 21, engine_icon(ename), C_GRAY);
        x += 14;
    }
    fit(en, ename, &FONT_S, 90 - (x - 4));
    x = cv_text(x, 20, &FONT_S, en, C_HI);
    {
        char pf[16];
        int32_t room = 236 - text_w(&FONT_S, ti) - 10 - (x + 10);
        str_cpy(pf, pn, sizeof pf);
        while (pf[0] && text_w(&FONT_S, pf) > room)
            pf[str_len(pf) - 1u] = 0;
        cv_text(x + 10, 20, &FONT_S, pf, pal_ribbon ? C_WHITE : C_AMB);
    }
    if (pal_ribbon) {                                   /* K-GLITCH: the page as a tag */
        int32_t tw = text_w(&FONT_T, ti) + 10;
        cv_rect(236 - text_w(&FONT_S, ti), 18, text_w(&FONT_S, ti) + 4, 20, C_BLACK);
        cv_card(236 - tw, 21, tw, 15, pal_card, pal_card);
        cv_text(236 - tw + 5, 23, &FONT_T, ti, C_GRAY);
    } else
        cv_text(236 - text_w(&FONT_S, ti), 20, &FONT_S, ti, C_GRAY);
    cv_blit(0, Y_FOOT);
}
UI_SMALL static void draw_columns(void)
{
    uint32_t c;
    char val[12];
    const char *unit;
    if (ui.home) {
        for (c = 0; c < 4u; c++) {
            int16_t *vp;
            const param_desc_t *d = home_param(c, &vp);
            param_format(d, *vp, val, &unit);
            draw_column(c, d->label, val, unit, VAL(c), RATIO(d, *vp), param_icon(d, *vp));
        }
        return;
    }
    if (is_drum(TSEL) && !page_for_drum(cur_page())) {   /* "DRUM TRACK" (the graph says so) */
        for (c = 0; c < 4u; c++)
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_TRK) {                 /* TRACK LEVEL LEN PAN of the selected track */
        const track_t *t = TSEL;
        uint32_t lvl = trk_level(song.sel);
        fmt_int(val, (int32_t)song.sel + 1);
        draw_column(0, "TRACK", val, "/4", VAL(0u), (int32_t)song.sel * 1000 / (NTRK - 1), ICON_AUTO);
        if (!lvl || t->p[P_MUTE]) {                    /* (MUTE: a turn of KNOB 2 unmutes, tracks_edit) */
            str_cpy(val, "MUTE", 12);
            unit = "";
        } else if (is_drum(t)) {
            fmt_int(val, (int32_t)lvl);
            unit = "";
        } else {
            param_format(&TP[P_LEVEL], (int32_t)lvl, val, &unit);
        }
        draw_column(1, "LEVEL", val, unit, lvl && !t->p[P_MUTE] ? VAL(1u) : C_DIM, (int32_t)lvl * 1000 / 127, ICON_AUTO);
        param_format(&TP[P_SLEN], t->p[P_SLEN], val, &unit);
        draw_column(2, "LEN", val, unit, VAL(2u), RATIO(&TP[P_SLEN], t->p[P_SLEN]), ICON_AUTO);
        param_format(&TP[P_PAN], t->p[P_PAN], val, &unit);
        draw_column(3, "PAN", val, unit, VAL(3u), RATIO(&TP[P_PAN], t->p[P_PAN]), param_icon(&TP[P_PAN], t->p[P_PAN]));
        return;
    }
    if (cur_page()->graph == GR_BROWSE) {
        uint32_t total, cur = preset_pos(&total);
        char u[8];
        fmt_int(val, (int32_t)cur + 1);
        str_cpy(u, "/", 8);
        fmt_int(u + 1, (int32_t)total);
        draw_column(0, "No.", val, u, VAL(0u), -1, ICON_NONE);
        draw_column(1, "ENG", ENGINES[TSEL->eng_req]->name, "", VAL(1u), -1, engine_icon(ENGINES[TSEL->eng_req]->name));
        draw_column(2, "", "", "", C_HI, -1, ICON_AUTO);
        draw_column(3, "FAV", fav_has(fav_key()) ? "ON" : "--", "", VAL(3u), -1, ICON_NONE);   /* (2.7) KNOB 4 */
        return;
    }
    if (cur_page()->graph == GR_USER) {                  /* SLOT, then three GO buttons */
        int used = up_used(ui.uslot);
        up_slot_label(val, ui.uslot);
        draw_column(0, "SLOT", val, "", VAL(0u), (int32_t)ui.uslot * 1000 / (int32_t)(UP_SLOTS - 1u), ICON_AUTO);
        draw_column(1, "LOAD", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(2, "ERASE", "--", "", used ? C_HI : C_DIM, -1, ICON_AUTO);
        draw_column(3, "SAVE", "--", "", C_HI, -1, ICON_AUTO);
        return;
    }
    if (cur_page()->scope == SC_STEP) {                 /* (2.7.2) NOTES: NOTE PITCH LEN FORCE of the note under edit */
        const step_t *st = &TSEL->step[ui.cursor];
        char sn[8], sl[8], u[8], ln[8];
        int on = step_on(st);
        uint32_t k = on ? ui.note_k % st->n : 0u;
        fmt_int(sn, (int32_t)ui.cursor + 1);
        str_cpy(sl, "/", 8);
        fmt_int(sl + 1, TSEL->p[P_SLEN]);
        draw_column(0, "NOTE", sn, sl, VAL(0u), -1, ICON_AUTO);
        u[0] = 0;
        if (on) {
            note_name(val, st->note[k]);
            if (st->n > 1u) {                             /* a chord: which of its notes */
                fmt_int(u, (int32_t)k + 1);
                str_cpy(u + str_len(u), "/", 2);
                fmt_int(u + str_len(u), st->n);
            }
        } else {
            str_cpy(val, "--", 12);
        }
        draw_column(1, "PITCH", val, u, on ? VAL(1u) : C_DIM, -1, ICON_AUTO);
        if (on)
            fmt_int(ln, (int32_t)note_span(TSEL, ui.cursor));
        else
            str_cpy(ln, "--", 8);
        draw_column(2, "LEN", ln, on ? "STEP" : "", on ? VAL(2u) : C_DIM, -1, ICON_AUTO);
        draw_column(3, "FORCE", on ? FORCE_N[note_force(st, k)] : "--", "", on ? VAL(3u) : C_DIM, -1, ICON_AUTO);
        return;
    }
    for (c = 0; c < 4u; c++) {
        int16_t *vp;
        const param_desc_t *d = page_desc(cur_page(), c, &vp);
        if (cur_page()->graph == GR_NAME) {          /* K-GLITCH NAME: K1 the place, K2 a digit / sign, K3 delete, K4 clear */
            static const char *const NL[4] = {"POS", "0-9", "DEL", "CLEAR"};
            char pv[4];
            fmt_int(pv, (int32_t)name_pos + 1);
            draw_column(c, NL[c], c == 0u ? pv : "--", "", C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->graph == GR_SLOTS && c == 1u) {   /* 2.6: ERASE, lit while the slot holds a project */
            draw_column(1, "ERASE", "--", "", project_used((uint32_t)song.g[G_SLOT] - 1u) ? C_HI : C_DIM, -1, ICON_AUTO);
            continue;
        }
        if (!d || !d->label || d->label[0] == '-') {
            draw_column(c, "", "", "", C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL && !usb.config) {   /* not connected: the
                                                       * USB status instead of the CPU (2.4: MIDI is MIDI OUT, ROUT is IN) */
            str_cpy(val, !usb.up ? "OFF" : usb.config ? "MIDI" : usb.setups ? "ENUM" : usb.sof_seen ? "BUS" : "WAIT", 12);
            unit = "USB";
            draw_column(c, "USB", val, unit, C_HI, -1, ICON_AUTO);
            continue;
        }
        if (cur_page()->id[c] == G_INFO && cur_page()->scope == SC_GLOBAL) {
            fmt_int(val, (int32_t)(song.cpu_q8 * 100u / 256u));
            unit = "%";
        } else {
            param_format(d, *vp, val, &unit);
        }
        draw_column(c, d->label, val, unit, VAL(c), d->fmt == F_ENUM && d->max < 2 ? -1 : RATIO(d, *vp),
                    param_icon(d, *vp));
    }
}


/* K-GLITCH: the page picker: the section's pages, the most visited first, the one ALGORITHM is on an orange card */
UI_SMALL static void pick_draw(void)
{
    static const char *const FN[FAM_COUNT] = {"HOME", "ENV", "LFO", "FX", "SEL", "EDIT", "GLO", "SAVE", "ARP", "SEQ", "TRACKS"};
    static uint32_t last;
    uint32_t sig = pick.sel * 131u + pick.n * 7u + pick.fam * 1009u + 1u, band, i, top;
    if (!ui.force && sig == last)
        return;
    last = sig;
    top = pick.sel > 5u ? pick.sel - 5u : 0u;
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 120, C_BLACK);
        cv_oy = -(int32_t)band * 120;
        cv_card(3, 2, 60, 15, C_WHITE, C_WHITE);
        cv_text(9, 3, &FONT_T, "GO TO", C_BLACK);
        cv_text(70, 1, &FONT_S, FN[pick.fam % FAM_COUNT], C_AMB);
        cv_rect(0, 21, 240, 1, C_WHITE);
        for (i = top; i < pick.n && i < top + 8u; i++) {
            int32_t y = 26 + (int32_t)(i - top) * 24, on = i == pick.sel;
            const page_t *pg = &PAGES[pick.idx[i]];
            const char *t = pg->scope == SC_ENGINE ? ENGINES[TSEL->eng_req % NENGINES]->page_title[pg->id[0] != P_E0] : pg->title;
            cv_card(6, y, 228, 21, on ? C_WHITE : pal_card, on ? C_WHITE : pal_card);
            cv_text(14, y + 2, &FONT_S, t, on ? C_BLACK : C_HI);
            if (page_visits[pick.idx[i]] >= 3u)       /* (often visited: a star) */
                cv_text(210, y + 4, &FONT_T, "*", on ? C_BLACK : C_WHITE);
            if (pick.idx[i] == ui.page && !ui.home)
                cv_text(170, y + 4, &FONT_T, "HERE", on ? C_BLACK : C_DIM);
        }
        cv_text(10, 222, &FONT_T, "ALGO: PICK  /  LET GO: OPEN", C_GRAY);
        cv_oy = 0;
        cv_blit(0, band * 120u);
    }
}

/* the UI's timers, once a frame whatever the screen (a message, the BPM highlight, an armed GO, the
 * white value of the knob just turned) */
UI_SMALL static void ui_timers(void)
{
    if (ui.msg_t)
        ui.msg_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.arm_t && !--ui.arm_t)
        ui.arm = 0;
    if (ui.hot_t)
        ui.hot_t--;
}

#include "kg_ui.c"                               /* K-GLITCH: DESTROY, D-SEQ, D-SETUP drawn whole */
static void vis_draw(void);                     /* ui_vis.c */
static int vis_shown(void);
static uint8_t vis_on, vis_shown_last, vis_name_t;   /* the visualiser (ui_vis.c): on, drawn last frame, its name's time */
/* K-GLITCH 1.0: HELP (HOME held 0.7 s): what the page does, what SHIFT (HOME held) does on it, the keys common to
 * every page; and SHIFT's line at the foot the moment HOME goes down. By kind of page (help_kind) */
enum { HK_GRID, HK_PADS, HK_FILTER, HK_KG, HK_TRACK, HK_SAVE, HK_SEQ, HK_DRUMS, HK_GLOBAL, HK_N };
static const char *const HELP_TXT[HK_N][4] = {
    {"4 TRACKS, 16 STEPS EACH, THE FX LANE", "ALGO: THE TRACK - THE KEYS PLAY IT", "K1 ENGINE  K2 LEVEL  K3 STEPS  K4 PAN",
     "REC: RECORD - FX: THE 16 EFFECT PADS"},
    {"WHITE KEYS: THE 16 EFFECT PADS", "K1: THE BANK - PRESETS: INTENSITY", "REC + PLAY: THE PADS ARE RECORDED",
     "OCT- HELD: ERASE AS IT PLAYS - OCT+: DICE"},
    {"A FILTER ON THE WHOLE MIX, 14 TYPES", "THE KEYS: ITS NOTE, IN TUNE", "REC + PLAY: THE KEYS ARE RECORDED",
     "NEXT PAGE, COLOR: DRIVE AND DELAY MODE"},
    {"THE ORANGE LINE AT THE FOOT SAYS HOW", "K1..K4: THE FOUR VALUES", "A 'GO' VALUE: TURN TWICE TO DO IT",
     "ITS BUTTON AGAIN: THE NEXT PAGE"},
    {"K1..K4: THE TRACK'S FOUR VALUES", "REC + PLAY + A KNOB: AUTOMATION,", "ITS VALUE KEPT ON EACH STEP",
     "ALGO: THE TRACK - BUTTON AGAIN: NEXT PAGE"},
    {"PRESETS: K1 SOUND  K2 ENGINE  K4 FAVOURITE", "SONGS: K1 A..P  K2 ERASE  K3 LOAD  K4 SAVE", "NAME: THE KEYS ARE LETTERS",
     "TOOLS: CLEAR, INIT THE SOUND, NEW SONG"},
    {"NOTES: A STEP'S NOTES, LENGTH, FORCE", "PATTERN: LENGTH, DIVISION, SWING, GATE", "SEQ HELD + WHITE KEYS: THE STEPS",
     "SEQ HELD + PRESETS: TOOLS, OCT+ DOES IT"},
    {"WHITE KEYS: THE 16 DRUM SOUNDS", "SEQ: THE GRID, THE KEYS ARE ITS STEPS", "EDIT: THE FM AND C64 KITS' SOUNDS",
     "REC: RECORD - EDIT HELD + KEY: ERASE"},
    {"K1..K4: FOUR VALUES OF THE WHOLE SONG", "A 'GO' VALUE: TURN TWICE TO DO IT", "ALGO: THE TRACK",
     "ITS BUTTON AGAIN: THE NEXT PAGE"},
};
static const char *const HELP_SH[HK_N][2] = {
    {"+ WHITE KEY: ITS STEP ON / OFF", "+ K2 OR K4: ITS AUTOMATION OFF"},
    {"+ PAD: ITS EFFECT OUT OF THE SEQ", "+ OCT-: THE FX SEQUENCE CLEARED"},
    {"+ ANY KEY: FILTER AUTOMATION OFF", "+ KNOB: BACK TO ITS DEFAULT"},
    {"+ KNOB: BACK TO ITS DEFAULT", "+ OCT-: THE FX SEQUENCE CLEARED"},
    {"+ KNOB: ITS AUTOMATION OFF", "+ OCT-: ALL THE TRACK'S AUTOMATION OFF"},
    {"+ OCT-: THE TRACK'S AUTOMATION OFF", "+ GLO: THE MENU"},
    {"+ OCT-: THE TRACK'S AUTOMATION OFF", "+ GLO: THE MENU"},
    {"+ GLO: THE MENU", "+ OCT-: (SYNTH TRACKS) AUTOMATION OFF"},
    {"+ KNOB: BACK TO ITS DEFAULT", "+ GLO: THE MENU"},
};
UI_SMALL static uint32_t help_kind(void)
{
    const page_t *pg = cur_page();
    if (ui.home || pg->scope == SC_TRK) return HK_GRID;
    if (pg->scope == SC_DRUM) return HK_DRUMS;
    if (pg->graph == GR_KG) return pg->id[0] == G_KFTYPE ? HK_FILTER : pg->fam == FAM_FX ? HK_PADS : HK_KG;
    if (pg->fam == FAM_SAVE) return HK_SAVE;
    if (pg->fam == FAM_SEQ) return HK_SEQ;
    return pg->scope == SC_GLOBAL ? HK_GLOBAL : HK_TRACK;
}
UI_SMALL static void help_draw(void)
{
    uint32_t k = help_kind(), pass, i;
    char t[32];
    str_cpy(t, "HELP  ", sizeof t);
    str_cpy(t + 6, ui.home || cur_page()->scope == SC_TRK ? "GRID" : cur_page()->title, sizeof t - 6u);
    for (pass = 0; pass < 2u; pass++) {
        int32_t y = 26;
        cv_begin(240, 120, C_BLACK);
        cv_oy = pass ? -120 : 0;
        cv_card(2, 2, 236, 236, pal_row, C_WHITE);
        cv_text(10, 5, &FONT_S, t, C_WHITE);
        for (i = 0; i < 4u; i++, y += 16) cv_text(10, y, &FONT_T, HELP_TXT[k][i], C_HI);
        cv_rect(10, y + 3, 220, 1, C_WHITE);
        cv_text(10, y + 6, &FONT_S, "SHIFT = HOLD HOME", C_WHITE);
        y += 27;
        for (i = 0; i < 2u; i++, y += 16) cv_text(10, y, &FONT_T, HELP_SH[k][i], C_HI);
        cv_text(10, y, &FONT_T, "+ GLO: MENU   + OCT-: CLEAR AUTOMATION", C_GRAY);
        cv_rect(10, y + 19, 220, 1, C_LINE);
        cv_text(10, y + 23, &FONT_T, "HOLD A BUTTON + ALGO: PICK ITS PAGE", C_GRAY);
        cv_text(10, y + 39, &FONT_T, "EDIT OR SAVE + OCT- / OCT+: UNDO / REDO", C_GRAY);
        cv_text(10, y + 55, &FONT_T, "REC + PLAY + PAD / KNOB: AUTOMATION", C_GRAY);
        cv_oy = 0;
        cv_blit(0, pass * 120u);
    }
}
UI_SMALL static void help_banner(void)                  /* HOME down: SHIFT's first action on this page, at the foot */
{
    char b[48];
    str_cpy(b, "SHIFT ", sizeof b);
    str_cpy(b + 6, HELP_SH[help_kind()][0], sizeof b - 6u);
    cv_begin(240, 15, C_WHITE);
    cv_text(6, 1, &FONT_T, b, C_BLACK);
    cv_blit(0, 225);
}
UI_SMALL static void ui_draw_page(void)
{
    ui.frame++;
    if (ui.home || ui.menu || cur_page()->graph != GR_KG)
        kg_pads = 0;                                    /* (K-GLITCH: the keys are pads only on its pages: kg_screen_draw) */
    name_pads = !ui.home && !ui.menu && cur_page()->graph == GR_NAME;
    pads_tick();
    if (rec_go) {                                       /* the take started: say so */
        rec_go = 0;
        ui_message("RECORDING");
    }
    if (ft_bars) {                                      /* a free take closed: the loop it made */
        char m[24];
        uint32_t n = ft_bars;
        ft_bars = 0;
        if (n == 0xFFu) {
            ui_message("TAKE DROPPED");
        } else {
            str_cpy(m, n == 1u ? "LOOP 1 BAR " : n == 2u ? "LOOP 2 BARS " : "LOOP 4 BARS ", sizeof m);
            fmt_int(m + str_len(m), song.g[G_BPM]);
            str_cpy(m + str_len(m), " BPM", sizeof m - str_len(m));
            ui_message(m);
        }
    }
    if (er_flash) {                                     /* EDIT + key took something out */
        er_flash = 0;
        if (!ui.msg_t)
            ui_message("ERASED");
    }
    if (pick.on && !ui.menu) {                          /* K-GLITCH: the page picker (a section's button held, ALGORITHM) */
        pick_draw();
        layer_shown = 1;                                /* (let go: the page drawn whole again) */
        ui_timers();
        ui.force = 0;
        return;
    }
    if (!ui.menu && (ui.layer != LY_PLAY || ui.hold_kind)) {   /* a layer held / a hold to confirm */
        if (ui.hold_kind)
            hold_screen_draw();
        else
            layer_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (layer_shown) {                                  /* back to the page */
        layer_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if ((rec_wait || ft_on) && !ui.menu && !on_song_page()) {   /* armed / free take */
        rec_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (rec_shown) {                                    /* back to the page */
        rec_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (ui_help && !ui.menu) {                          /* K-GLITCH 1.0: the HELP sheet, while HOME is held */
        if (ui.force)
            help_draw();
        ui.force = 0;
        return;
    }
#if FELUCCA_ARRANGER
    if (!ui.menu && on_song_page()) {
        song_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
#endif
    if (ui.menu) {
        draw_menu();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown()) {                                  /* the visualiser over the TRACKS screen */
        vis_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (vis_shown_last) {                               /* back from it */
        vis_shown_last = 0;
        vis_on = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    if (!ui.home && cur_page()->scope == SC_TRK) {
        studio_tracks_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (on_drum_page()) {
        drum_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    kgd_ui_sync();                                      /* (K-GLITCH DRUMS: EDIT's values, the sound's) */
    if (!ui.home && cur_page()->graph == GR_KG) {      /* K-GLITCH: its own screen (kg_ui.c) */
        kg_screen_draw();
        ui_timers();
        ui.force = 0;
        return;
    }
    if (kg_shown) {                                     /* back from it */
        kg_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.force = 1;
    }
    cursor_fix();
    if (ui.force)
        draw_frame();
    felucca_dbg.stage = 3;
    draw_head();
    felucca_dbg.stage = 4;
    draw_columns();
    felucca_dbg.stage = 5;
    draw_graph();
    ui_timers();
    felucca_dbg.stage = 6;
    draw_foot();
    ui.force = 0;
}
UI_SMALL static void ui_draw(void)
{
    ui_draw_page();
    if (shift_dn && !ui_help && !ui.menu)               /* (over the page: SHIFT's line) */
        help_banner();
}
