/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: HOME held on PERFORM or GENERATOR: in the slice blocks' place, what leaves the mix (the break, the
 * kick, the drums, the effects: before MASTER) scrolling in from the right, as a track on a CDJ, the playhead at the
 * right edge: each column its lows, mids and highs in rekordbox's RGB (the lows red, the mids green, the highs blue,
 * mixed), as tall as its peak; the beats ticked, the bar's first lit. The columns are the render's (app.c's scope), a
 * 15th of a step each, so 4 beats are 240 columns. (Part of app.c's unity: included there in order.) */

#define CDJ_PX 15                        /* columns a step (16th) */
static void cdj_picture(const brk_status_t *st, uint32_t w, int32_t top)
{
    int32_t x, k, mid = 52 - top, s16 = st->bar16 >= 0 ? st->bar16 : 0;
    uint32_t z = CDJ_Z[set.wzoom % CDJ_ZN];
    float ph = st->playing ? st->phase : 0.0f, spx = (float)CDJ_PX * 12.0f / (float)z;   /* (pixels a step) */
    for (x = 0; x < 240; x++) {          /* the newest at the right */
        uint32_t back = (239u - (uint32_t)x) * z / 12u, m;
        const uint8_t *v;
        int32_t h;
        if (back >= w || back >= SCOPE_N)
            continue;
        v = scope[(w - 1u - back) % SCOPE_N];
        m = v[0] > v[1] ? v[0] : v[1];
        if (v[2] > m)
            m = v[2];
        h = (int32_t)((uint32_t)v[3] * 48u / 255u);
        if (!m || !h)
            continue;
        m = 110u + (uint32_t)v[3] * 145u / 255u;   /* (the bands' mix at full brightness, a touch dimmer when quiet) */
        wave_col(x, mid, h, v[0], v[1], v[2], m, ws_style(), 0);   /* (WAVE TYPE: RGB, SERATO, 3 BAND, BLUE, 0.9.70) */
    }
    for (k = 0; k < 9; k++) {            /* the beats back from now: a tick top and bottom, the bar's first lit */
        int32_t at = s16 - s16 % 4 - 4 * k;
        int32_t xb = 239 - (int32_t)((float)(s16 % 4 + 4 * k) * spx + ph * spx);
        uint16_t c = ((at % 16) + 16) % 16 ? C_GRAY : C_HI;
        if (xb < 0)
            break;
        cv_rect(xb, 0 - top, 1, 5, c);
        cv_rect(xb, 99 - top, 1, 5, c);
    }
    cv_rect(237, 0 - top, 3, 104, C_WHITE);   /* the playhead, at the edge */
    if (!top) {                          /* LOAF BREAKER TEST: a track playing: how far along it (a line at the top), and
                                          * the bars left to its next DROP */
        uint32_t done, total;
        int32_t din;
        if (trk_where(st, &done, &total, &din)) {
            cv_rect(0, 0, 240, 3, C_LINE);
            if (total)
                cv_rect(0, 0, (int32_t)(done * 240u / total), 3, C_HI);
            if (din > 0 && din <= 8) {
                char d[16] = "DROP IN ";
                int_text(din, d + 8);
                cv_text(234 - text_w(&FONT_XS, d), 6, &FONT_XS, d, din <= 1 ? C_WHITE : C_AMB);
            }
        }
    }
    if (!top) {                          /* the bar and the beat, as a CDJ counts them */
        char t[12], u[4];
        int_text((int32_t)(st->bar % 999u) + 1, t);
        text_cat(t, ".");
        int_text(s16 / 4 + 1, u);
        text_cat(t, u);
        cv_text(4, 6, &FONT_XS, t, C_WHITE);
    }
}
static void draw_cdj(const brk_status_t *st)
{
    uint32_t row, w = scope_w;
    for (row = 0; row < 2u; row++) {     /* (y 88..191, two strips) */
        cv_begin(240, 52, C_BLACK);
        cdj_picture(st, w, (int32_t)row * 52);
        cv_blit(0, 88u + row * 52u);
    }
}
