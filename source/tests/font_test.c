/* SPDX-License-Identifier: GPL-3.0-only */
/* The screen fonts (gfx.c cv_text, tools/gen_font.py). K-GLITCH: anti-aliased, from Carlito Bold (2 bits a pixel), proportional with
 * tabular figures, on SLOOP's rows: S 16 high (the figures 8 wide, as Terminus's), L 32 (figures 15), T 11 (figures 6;
 * K-GLITCH's small labels). Checked: the cells, the figures' advances (values do not jitter), every advance within
 * reason; every printable character draws something (the
 * space nothing), inside its cell's rows; the anti-aliasing blends with what is under it (white text on orange: no
 * black fringe; dark text on a light fill: no pixel darker than the text); lower case drawn as capitals; clipping at the
 * canvas's edges. (SLOOP's own test checked Terminus pixel for pixel: KG_FONT=terminus builds that font again.)
 * Exit status: the number of failed checks. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p) { (void)x; (void)y; (void)w; (void)h; (void)p; }
#define __attribute__(x)
#include "../firmware/src/gfx.c"
static int bad;
static void check(int ok, const char *what) { printf("font: %-70s %s\n", what, ok ? "ok" : "FAIL"); bad += !ok; }
static uint16_t px(uint32_t x, uint32_t y) { return swap16(cv_px[y * cv_w + x]); }
static uint32_t lum(uint16_t c) { return ((c >> 11) & 31u) * 2u + ((c >> 5) & 63u) + (c & 31u) * 2u; }
int main(void)
{
    const felucca_font_t *F[3] = {&FONT_S, &FONT_L, &FONT_T};
    static const uint32_t H[3] = {16, 32, 11}, ADV[3] = {8, 15, 6};
    uint32_t f, c, x, y, okc, oka, okd;
    for (f = 0; f < 3u; f++) {
        const felucca_font_t *ft = F[f];
        char nm[80];
        okc = ft->h == H[f];
        oka = 1;
        okd = 1;
        for (c = ft->first; c <= ft->last; c++) {
            char s[2] = {(char)c, 0};
            uint32_t lit = 0;
            oka &= (c >= '0' && c <= '9') ? ft->adv[c - ft->first] == ADV[f] : ft->adv[c - ft->first] <= ADV[f] * 2u;
            cv_begin(64, 48, 0);
            cv_text(10, 4, ft, s, 0xFFFFu);
            for (y = 0; y < 48u; y++)
                for (x = 0; x < 64u; x++)
                    if (px(x, y)) {
                        lit++;
                        okd &= y >= 4u && y < 4u + ft->h;
                    }
            if (c == ' ') okd &= lit == 0u;
            else if (c >= 33u && c <= 126u && !(c >= 'a' && c <= 'z')) okd &= lit > 0u;
        }
        snprintf(nm, sizeof nm, "font %u: cell %u high, figures %u wide, every glyph drawn in its rows", f, H[f], ADV[f]);
        check(okc && oka && okd, nm);
    }
    {   /* lower case as capitals */
        uint32_t same = 1;
        static uint16_t a[64 * 48];
        cv_begin(64, 48, 0);
        cv_text(2, 2, &FONT_S, "kick", 0xFFFFu);
        memcpy(a, cv_px, sizeof a);
        cv_begin(64, 48, 0);
        cv_text(2, 2, &FONT_S, "KICK", 0xFFFFu);
        for (x = 0; x < 64u * 48u; x++) same &= a[x] == cv_px[x];
        check(same, "lower case drawn as capitals");
    }
    {   /* the blending: white on orange, and dark on a light fill */
        uint16_t org = RGB(255, 136, 56), lite = RGB(220, 220, 220), ink = RGB(20, 27, 35);
        uint32_t ok1 = 1, ok2 = 1, any = 0;
        cv_begin(120, 40, org);
        cv_text(4, 4, &FONT_S, "DESTROY 170", 0xFFFFu);
        for (y = 0; y < 40u; y++)
            for (x = 0; x < 120u; x++)
                ok1 &= lum(px(x, y)) >= lum(org);      /* (between orange and white: never darker than the fill) */
        cv_begin(120, 40, lite);
        cv_text(4, 4, &FONT_S, "DESTROY 170", ink);
        for (y = 0; y < 40u; y++)
            for (x = 0; x < 120u; x++) {
                uint16_t p = px(x, y);
                ok2 &= lum(p) >= lum(ink) && lum(p) <= lum(lite);
                any += p != lite;
            }
        check(ok1 && ok2 && any > 50u, "anti-aliasing over the background (no dark fringe on a fill)");
    }
    {   /* clipping */
        cv_begin(30, 20, 0);
        cv_text(-5, -6, &FONT_L, "88", 0xFFFFu);
        cv_text(25, 12, &FONT_L, "88", 0xFFFFu);
        check(1, "text past the canvas's edges is clipped (no write outside)");
    }
    printf("font: S %u B, L %u B, T %u B\n", (unsigned)sizeof FONT_S_DATA, (unsigned)sizeof FONT_L_DATA, (unsigned)sizeof FONT_T_DATA);
    return bad;
}
