/* SPDX-License-Identifier: GPL-3.0-only */
/* SLOOP boot splash: the logo (tools/gen_logo.py -> sloop_logo.h: 16 colours, RLE runs of
 * (run - 1) << 4 | colour), drawn into the canvas in bands of at most 120 rows. */
#ifndef UI_SMALL                 /* (2.7) compiled for size: not the audio path (gfx.c UI_SMALL); the host tests: nothing */
#define UI_SMALL
#endif
#ifndef FELUCCA_LOGO_TEXT       /* (2.7) 1: the boot logo as text, "S" and "LOOP" in the four track colours (4.6 KB of flash
                                 * less: tools/build.py builds so by itself when the flash is full) */
#define FELUCCA_LOGO_TEXT 0
#endif
#if FELUCCA_LOGO_TEXT
UI_SMALL static void sloop_logo_draw(uint32_t y0)
{
    static const char L[5] = {'S', 'L', 'O', 'O', 'P'};
    uint32_t i, w = text_w(&FONT_L, "SLOOP"), k = w * 3u <= 232u ? 3u : 2u, h = 32u, off, y, x;
    int32_t px = 0;
    char c[2] = {0, 0};
    cv_begin(240, h * k, C_BLACK);
    for (i = 0; i < 5u; i++) {                          /* small, at the top left: S white, L O O P the tracks' colours */
        c[0] = L[i];
        px = cv_text(px, 0, &FONT_L, c, i ? TE_COL_DEF[i - 1u] : C_WHITE);
    }
    off = (240u - w * k) / 2u;
    for (y = h * k; y-- > 0;)                           /* then k times as big, in place (from the end: nothing read is
                                                         * overwritten before) */
        for (x = w * k; x-- > 0;)
            cv_px[y * 240u + off + x] = cv_px[(y / k) * 240u + x / k];
    for (y = 0; y < h * k; y++)                         /* (the small one's left-over columns) */
        for (x = 0; x < off; x++)
            cv_px[y * 240u + x] = swap16(C_BLACK);
    cv_blit(0, y0 + 60u);
}
#else
#include "sloop_logo.h"
UI_SMALL static void sloop_logo_draw(uint32_t y0)
{
    uint32_t r0;
    for (r0 = 0; r0 < SLOOP_SPLASH_H; r0 += 120u) {
        uint32_t rows = SLOOP_SPLASH_H - r0 > 120u ? 120u : SLOOP_SPLASH_H - r0;
        uint32_t lo = r0 * SLOOP_SPLASH_W, hi = (r0 + rows) * SLOOP_SPLASH_W, pos = 0, i;
        cv_begin(SLOOP_SPLASH_W, rows, C_BLACK);
        for (i = 0; i < sizeof SLOOP_SPLASH_RLE && pos < hi; i++) {
            uint32_t run = (SLOOP_SPLASH_RLE[i] >> 4) + 1u, c = SLOOP_SPLASH_RLE[i] & 15u, p;
            if (c && pos + run > lo)
                for (p = pos < lo ? lo : pos; p < pos + run && p < hi; p++)
                    cv_px[p - lo] = swap16(SLOOP_SPLASH_PAL[c]);
            pos += run;
        }
        cv_blit((240u - SLOOP_SPLASH_W) / 2u, y0 + r0);
    }
}
#endif

/* K-GLITCH: the boot screen. The wordmark "K-GLITCH" drawn small, three times (track 1's blue a pixel left,
 * track 4's orange a pixel right, white on top), then 7/4 times as big in place (224 x 56: the colours split
 * like a broken signal), two bands of rows pushed sideways (the glitch), and under it the version and where it
 * comes from: SLOOP (isod89), Felucca (Hügelton Instruments). sloop_logo_draw stays for FELUCCA_LOGO_TEXT. */
UI_SMALL static void kg_wordmark(uint32_t y0)
{
    static const struct { uint8_t y, h; int8_t dx; } SL[3] = {{17, 5, 7}, {36, 3, -6}, {47, 2, 4}};
    uint32_t i, x, y, w = text_w(&FONT_L, "K-GLITCH"), W = w * 7u / 4u, H = 56u, off;
    cv_begin(240, H, C_BLACK);
    cv_text(0, 0, &FONT_L, "K-GLITCH", TE_COL_DEF[0]);
    cv_text(2, 0, &FONT_L, "K-GLITCH", TE_COL_DEF[3]);
    cv_text(1, 0, &FONT_L, "K-GLITCH", RGB(255, 255, 255));
    if (W > 236u) W = 236u;
    off = (240u - W) / 2u;
    for (y = H; y-- > 0;)                               /* 7/4 as big, in place, from the end (as sloop_logo_draw) */
        for (x = W; x-- > 0;)
            cv_px[y * 240u + off + x] = cv_px[(y * 4u / 7u) * 240u + x * 4u / 7u];
    for (y = 0; y < H; y++)
        for (x = 0; x < off; x++)
            cv_px[y * 240u + x] = swap16(C_BLACK);
    for (i = 0; i < 3u; i++)                            /* the glitch: bands of rows pushed sideways */
        for (y = SL[i].y; y < (uint32_t)SL[i].y + SL[i].h; y++) {
            uint16_t row[240];
            for (x = 0; x < 240u; x++) {
                int32_t sx = (int32_t)x - SL[i].dx;
                row[x] = sx >= 0 && sx < 240 ? cv_px[y * 240u + (uint32_t)sx] : swap16(C_BLACK);
            }
            for (x = 0; x < 240u; x++) cv_px[y * 240u + x] = row[x];
        }
    cv_blit(0, y0);
}
UI_SMALL static void sloop_splash(void)
{
    const char *v = "K-GLITCH " KGLITCH_VERSION " / " FELUCCA_VERSION;
    (void)sloop_logo_draw;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    kg_wordmark(70);
    cv_begin(240, 56, C_BLACK);
    cv_text(120 - text_w(&FONT_S, "DRUM DESTROYER") / 2, 0, &FONT_S, "DRUM DESTROYER", RGB(255, 98, 26));
    cv_text(120 - text_w(&FONT_S, v) / 2, 22, &FONT_S, v, RGB(196, 196, 204));
    cv_text(120 - text_w(&FONT_S, "based on sloop + felucca") / 2, 40, &FONT_S, "based on sloop + felucca", RGB(96, 96, 104));
    cv_blit(0, 160);
}
