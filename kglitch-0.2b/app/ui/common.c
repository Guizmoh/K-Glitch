/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the UI's state, its text helpers, messages, tap tempo, the screen's colours and big text. (Part of
 * app.c's unity: included there in order.) */

/* the screen's maths: the engine's shared copies (brk_math.c), not an inline copy at every call (the flash is full) */
#define brk_exp2f brk_exp2f_c
#define brk_log2f brk_log2f_c

/* LOAF BREAKER: WAVE ZOOM (GLO MORE, PRESETS): how much of the past the screen's width holds, in twelfths of a bar
 * (cdj.c; each pixel back is CDJ_Z / 12 of the render's columns): 1/2, 2/3, 5/6, 1 (as it was), 1 1/4, 1 1/2, 2 bars */
#define CDJ_ZN 7u
static const uint8_t CDJ_Z[CDJ_ZN] = {6u, 8u, 10u, 12u, 15u, 18u, 24u};
static const char *const CDJ_ZNAME[CDJ_ZN] = {"1/2 BAR", "2/3 BAR", "5/6 BAR", "1 BAR", "1 1/4 BARS", "1 1/2 BARS",
                                             "2 BARS"};
/* LOAF BREAKER: the screen's EFFECTS (GLO MORE K3), K1 their AMOUNT: every blit of the UI's goes through here and the
 * canvas (a strip of at most 240 x 60, big-endian RGB565) is worked on before it goes out. CRT: every other row dimmed;
 * VHS: bands that slip sideways, more on a kick; GRAIN: specks that move; RGB: the red a few pixels off the rest;
 * SHAKE: the strip pushed aside on a kick, hard at the drop; STROBE: the colours turned over at the drop (and on the
 * kicks, past half); 8-BIT: fewer colours. The kick and the drop are seen through kick_hit_n and build_p (app.c) */
enum { SFX_OFF, SFX_CRT, SFX_VHS, SFX_GRAIN, SFX_RGB, SFX_SHAKE, SFX_STROBE, SFX_8BIT, SFX_N };
static const char *const SFX_NAME[SFX_N] = {"OFF", "CRT", "VHS", "GRAIN", "RGB", "SHAKE", "STROBE", "8-BIT"};
static uint32_t sfx_rand(void)
{
    static uint32_t x = 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}
static void sfx_slide(uint16_t *p, uint32_t w, int32_t d)   /* a row moved d pixels (black where it left) */
{
    int32_t i, n = (int32_t)w;
    if (d > 0) {
        for (i = n - 1; i >= 0; i--)
            p[i] = i >= d ? p[i - d] : 0u;
    } else if (d < 0) {
        for (i = 0; i < n; i++)
            p[i] = i - d < n ? p[i - d] : 0u;
    }
}
static uint32_t sfx_kick_t, sfx_drop_t;   /* the last kick, the last drop (plat_ms | 1; 0 none) */
static uint8_t sfx_splash_d;            /* LOAF BREAKER: the splash's chromatic aberration (its red this far off; 0 none) */
static uint32_t sfx_win(uint32_t ms)     /* a kick's or a drop's moment, by GLO EFFECTS' LENGTH (50: as given) */
{
    return ms * (50u + (set.sfx_len > 100u ? 100u : set.sfx_len)) / 100u;
}
#define SFX_ON(e) (sfx_splash_d ? sfx_splash_d < 255u && (e) == SFX_RGB : (set.sfx_on >> ((e) - 1u)) & 1u)
                                         /* (several at once; sfx_splash_d 255: none, PRESS PLAY's wave) */
static int sfx_live(uint32_t now)       /* a kick's or a drop's moment of SHAKE, STROBE or VHS (main.c redraws the
                                          * whole screen as it starts and ends, so no strip keeps it) */
{
    static uint32_t kseen;
    static float bp_was = -1.0f;
    uint32_t sk, sd, km, dm;
    if (kick_hit_n != kseen) {
        kseen = kick_hit_n;
        sfx_kick_t = now | 1u;
    }
    if (bp_was >= 0.0f && build_p < 0.0f)
        sfx_drop_t = now | 1u;           /* (the build just ended: the drop) */
    bp_was = build_p;
    sk = sfx_kick_t ? now - sfx_kick_t : 99999u;
    sd = sfx_drop_t ? now - sfx_drop_t : 99999u;
    km = set.sfx_kick && sk < sfx_win(100u);
    dm = set.sfx_drop && sd < sfx_win(300u);
    return ((SFX_ON(SFX_SHAKE) || SFX_ON(SFX_VHS)) && (km || dm)) ||
           (SFX_ON(SFX_STROBE) && ((set.sfx_drop && sd < sfx_win(160u)) || (set.sfx_kick > 50u && km)));
}
static uint32_t ui_blits;                /* LOAF BREAKER: the strips sent to the screen (GLO MORE's METER) */
/* LOAF BREAKER: what each screen row shows, as a hash of its pixels (0: not known): a full-width strip sends only its
 * rows that changed (none: nothing sent). A narrower strip, a fill, a page's whole redraw forget the rows they touch */
static uint32_t row_h[240];
static void rows_forget(uint32_t y, uint32_t h)
{
    uint32_t i;
    for (i = y; i < y + h && i < 240u; i++)
        row_h[i] = 0u;
}
static void rows_send(uint32_t x, uint32_t y)
{
    uint32_t r, r0 = cv_h, r1 = 0u, i;
    if (x || cv_w != 240u) {             /* (a narrow one: sent, its rows no longer known) */
        rows_forget(y, cv_h);
        ui_blits++;
        cv_blit(x, y);
        return;
    }
    for (r = 0; r < cv_h && y + r < 240u; r++) {
        const uint16_t *p = cv_px + r * 240u;
        uint32_t h = 2166136261u;
        for (i = 0; i < 240u; i += 2u)
            h = (h ^ ((uint32_t)p[i] | (uint32_t)p[i + 1u] << 16)) * 16777619u;
        h |= 1u;                         /* (never 0: 0 is "not known") */
        if (h != row_h[y + r]) {
            row_h[y + r] = h;
            r0 = r < r0 ? r : r0;
            r1 = r + 1u;
        }
    }
    if (r0 < r1) {
        ui_blits++;
        lcd_blit(x, y + r0, 240u, r1 - r0, cv_px + r0 * 240u);
    }
}
static void rib_frame(uint32_t x, uint32_t y);
static void crt_blit(uint32_t x, uint32_t y)   /* (the effects on, in this order: 8-BIT, GRAIN, CRT, RGB, VHS, SHAKE,
                                          * STROBE) */
{
    uint32_t amt = set.scrfx > 100u ? 100u : set.scrfx, now = plat_ms(), r, i;
    uint32_t sinc_kick = sfx_kick_t ? now - sfx_kick_t : 99999u, sinc_drop = sfx_drop_t ? now - sfx_drop_t : 99999u;
    uint32_t ks = set.sfx_kick > 100u ? 100u : set.sfx_kick, ds = set.sfx_drop > 100u ? 100u : set.sfx_drop;
    int km = ks && sinc_kick < sfx_win(100u), dm = ds && sinc_drop < sfx_win(300u);
    if (SFX_ON(SFX_8BIT)) {          /* 3 bits a colour, or 2 past half */
        uint16_t m = amt > 50u ? 0xC618u : 0xE71Cu;
        for (i = 0; i < cv_w * cv_h; i++)
            cv_px[i] = swap16((uint16_t)(swap16(cv_px[i]) & m));
    }
    if (SFX_ON(SFX_GRAIN)) {         /* specks: a pixel darkened or lit, amt / 8 % of them, more on a kick */
        uint32_t n = cv_w * cv_h * (amt + 4u) / 800u * (km ? 1u + ks / 50u : 1u);
        while (n--) {
            uint32_t at = sfx_rand() % (cv_w * cv_h);
            uint16_t c = swap16(cv_px[at]);
            cv_px[at] = swap16((sfx_rand() & 1u) ? (uint16_t)((c >> 1) & 0x7BEFu) : (uint16_t)(c | 0x2104u));
        }
    }
    if (SFX_ON(SFX_CRT)) {           /* the odd rows at 3/4, 1/2 or 1/4 */
        for (r = (y & 1u) ? 0u : 1u; r < cv_h; r += 2u) {
            uint16_t *p = cv_px + r * cv_w;
            for (i = 0; i < cv_w; i++) {
                uint16_t c = swap16(p[i]), h = (uint16_t)((c >> 1) & 0x7BEFu), q = (uint16_t)((c >> 2) & 0x39E7u);
                p[i] = swap16(amt < 34u ? (uint16_t)(h + q) : amt < 67u ? h : q);
            }
        }
    }
    if (SFX_ON(SFX_RGB)) {           /* the red 1..4 pixels right of the green and blue, more on a kick */
        uint32_t d = sfx_splash_d ? sfx_splash_d : 1u + amt / 34u + (km ? ks / 34u : 0u);
        for (r = 0; r < cv_h; r++) {
            uint16_t *p = cv_px + r * cv_w;
            for (i = cv_w; i-- > 0;) {
                uint16_t c = swap16(p[i]), red = i >= d ? (uint16_t)(swap16(p[i - d]) & 0xF800u) : 0u;
                p[i] = swap16((uint16_t)((c & 0x07FFu) | red));
            }
        }
    }
    if (SFX_ON(SFX_VHS)) {           /* a band or two slipping, more on a kick and at the drop */
        uint32_t bands = (km ? ks / 25u : 0u) + (dm ? 2u + ds / 20u : 0u) + (sfx_rand() % 100u < amt / 3u ? 1u : 0u);
        while (bands--) {
            uint32_t r0 = sfx_rand() % cv_h, n = 2u + sfx_rand() % (2u + amt / 10u);
            int32_t d = (int32_t)(1u + sfx_rand() % (1u + amt / 12u)) * ((sfx_rand() & 1u) ? 1 : -1);
            for (r = r0; r < cv_h && r < r0 + n; r++)
                sfx_slide(cv_px + r * cv_w, cv_w, d);
        }
    }
    if (SFX_ON(SFX_SHAKE) && (km || dm)) {   /* the strip pushed aside, hard at the drop */
        int32_t d = (int32_t)((1u + amt / 30u) * (dm ? 1u + ds / 25u : (ks + 49u) / 50u)) * ((now / 23u) & 1u ? 1 : -1);
        for (r = 0; r < cv_h; r++)
            sfx_slide(cv_px + r * cv_w, cv_w, d);
    }
    if (SFX_ON(SFX_STROBE) && ((ds && sinc_drop < sfx_win(160u)) || (ks > 50u && km)))
        for (i = 0; i < cv_w * cv_h; i++)
            cv_px[i] = (uint16_t)~cv_px[i];
    rib_frame(x, y);                     /* (K-GLITCH 0.2b, RIBBON: the page's card behind its content) */
    rows_send(x, y);                     /* (only the rows that changed) */
}
#define cv_blit crt_blit
static void scr_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    rows_forget(y, h);                   /* (the rows it covers no longer known) */
    lcd_fill(x, y, w, h, c);
}
#define lcd_fill scr_fill

/* ---------------------------------------------------------------- skins --- */
/* GLOUPI BREAKS: the screen's colours, picked on GLOBAL's MORE page (K1) and saved (set.skin). fm1-base's palette
 * (gfx.c pal[]: rules, dim, labels, secondary, values) and the app's own (a block's ground, the block sounding, the
 * white, TAP TEMPO's unlit segments and metronome) from one table */
#define NSKINS 25u                       /* (K-GLITCH 0.2b: + RIBBON, the 25th, the default) */
/* LOAF BREAKER (FLEXOR, 11 Nov 2026, 0.237: the 1.0's skins, picked, ordered and named by him on listening-page style
 * checklists from 48 rendered on the simulator; two rows of twelve swatches on SETUP). Saved as SKIN_NEW | its place:
 * an older save's place in the 18 before is mapped (SKIN_OLD) to the nearest kept */
#define SKIN_NEW 0x80u
#define SKIN_RIBBON 24u                  /* K-GLITCH 0.2b: RIBBON, K-WASUB's / K-GLITCH's look: a slate ground, cards, pills */
#define SKIN_DEFAULT SKIN_RIBBON
#define SKIN_LOAF_DEFAULT 10u                 /* (RED, LOAF BREAKER's own, 0.240; CHOPPAH 0.237-0.239) */
static const char *const SKIN_NAME[NSKINS] = {"CHROME", "FIRE&ICE", "CHOPPAH", "GLOUPI", "TESS", "OUTRUN", "THUNDER", "CYBERPUNK", "MC SALO", "ACID", "RED", "GARRY", "PIP-BOY", "NEON", "BLACKLIGHT", "PATINA", "SYNTHWAVE", "MIAMI", "ELECTRIC", "ARCTIC", "BREADCRUMBZ", "IRIS", "GLUCKSMAN", "MAXENCE", "RIBBON"};
static const uint8_t SKIN_OLD[18] = {10u, 2u, 19u, 12u, 21u, 23u, 15u, 20u, 0u, 23u, 16u, 17u, 11u, 17u, 3u, 6u, 3u, 9u};   /* (RED AMBER ICE-BLUE PIP-BOY IRIS
                                          * PINK MINT WHITE BLOOD MAXENCE SYNTHWAVE VAPOR TOXIC MIAMI LAVA GOLD BLAZE ACID) */
/* each: line, dim, gray, mid (secondary), hi (values), block, white */
static const uint8_t SKIN_RGB[NSKINS][7][3] = {
    {{41, 41, 48}, {101, 101, 116}, {198, 198, 208}, {255, 40, 60}, {235, 240, 250}, {21, 21, 24}, {253, 254, 254}}   /* CHROME: chrome white and hot red on black */,
    {{36, 36, 48}, {89, 89, 116}, {191, 191, 208}, {255, 60, 40}, {130, 230, 255}, {18, 18, 24}, {242, 252, 255}}   /* FIRE&ICE: icy cyan against fire red */,
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {232, 136, 30}, {255, 184, 70}, {24, 24, 30}, {255, 255, 255}}   /* CHOPPAH: AMBER until 0.236, CHOPPAH's own: orange */,
    {{48, 24, 20}, {100, 56, 44}, {190, 160, 150}, {230, 30, 60}, {255, 140, 20}, {24, 10, 8}, {255, 240, 220}}   /* GLOUPI: LAVA until 0.236: molten orange over crimson */,
    {{41, 14, 48}, {101, 39, 116}, {198, 159, 208}, {245, 60, 130}, {255, 196, 90}, {21, 7, 24}, {255, 249, 238}}   /* TESS: golden peach and magenta, a violet night */,
    {{21, 10, 48}, {54, 31, 116}, {169, 155, 208}, {180, 60, 255}, {255, 140, 40}, {10, 5, 24}, {255, 244, 234}}   /* OUTRUN: retro orange and violet on a blue night */,
    {{13, 17, 48}, {37, 47, 116}, {158, 165, 208}, {40, 120, 255}, {255, 240, 60}, {7, 9, 24}, {255, 254, 236}}   /* THUNDER: lightning yellow and electric blue */,
    {{10, 43, 48}, {30, 105, 116}, {154, 201, 208}, {0, 240, 255}, {255, 236, 40}, {5, 22, 24}, {255, 253, 234}}   /* CYBERPUNK: neon yellow and cyan on a teal night */,
    {{36, 48, 18}, {89, 116, 48}, {191, 208, 165}, {40, 255, 120}, {255, 225, 0}, {18, 24, 9}, {255, 252, 230}}   /* MC SALO: hazard yellow and radioactive green */,
    {{46, 42, 14}, {98, 92, 40}, {196, 190, 140}, {255, 64, 28}, {255, 234, 0}, {22, 20, 6}, {255, 255, 228}}   /* ACID: acid yellow, a red-orange beside it */,
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {232, 76, 60}, {255, 132, 112}, {24, 24, 30}, {255, 255, 255}}   /* RED: the red from the start, LOAF BREAKER's default (0.240, FLEXOR: in JUNGLE's place) */,
    {{24, 40, 20}, {60, 90, 50}, {160, 190, 150}, {170, 60, 255}, {190, 255, 40}, {10, 18, 8}, {235, 255, 220}}   /* GARRY: TOXIC until 0.239: acid green, poison purple */,
    {{14, 44, 22}, {30, 100, 54}, {80, 180, 110}, {40, 220, 100}, {140, 255, 170}, {8, 28, 14}, {200, 255, 210}}   /* PIP-BOY: an old terminal's green */,
    {{48, 48, 48}, {116, 116, 116}, {208, 208, 208}, {255, 30, 150}, {60, 255, 40}, {24, 24, 24}, {236, 255, 234}}   /* NEON: neon green and neon pink on pure black */,
    {{28, 12, 48}, {71, 35, 116}, {179, 157, 208}, {60, 255, 100}, {200, 100, 255}, {14, 6, 24}, {250, 240, 255}}   /* BLACKLIGHT: UV violet and fluorescent green */,
    {{48, 41, 34}, {116, 101, 85}, {208, 198, 188}, {225, 115, 55}, {60, 225, 195}, {24, 21, 17}, {236, 252, 249}}   /* PATINA: verdigris and copper */,
    {{44, 22, 82}, {96, 58, 150}, {186, 160, 226}, {255, 46, 170}, {40, 236, 255}, {20, 8, 44}, {255, 214, 236}}   /* SYNTHWAVE: neon cyan and hot magenta on indigo */,
    {{20, 40, 56}, {50, 90, 110}, {150, 190, 200}, {0, 210, 200}, {255, 110, 150}, {8, 22, 32}, {255, 240, 230}}   /* MIAMI: hot coral on deep teal, turquoise */,
    {{16, 16, 48}, {44, 44, 116}, {163, 163, 208}, {255, 40, 150}, {70, 170, 255}, {8, 8, 24}, {236, 246, 255}}   /* ELECTRIC: electric blue and hot pink */,
    {{13, 22, 48}, {37, 58, 116}, {158, 172, 208}, {60, 130, 255}, {190, 245, 255}, {6, 11, 24}, {248, 254, 255}}   /* ARCTIC: ice white and steel blue, a polar night */,
    {{34, 34, 40}, {74, 74, 82}, {150, 150, 158}, {196, 196, 202}, {236, 236, 240}, {24, 24, 30}, {255, 255, 255}}   /* BREADCRUMBZ: WHITE until 0.236: white and greys */,
    {{34, 32, 44}, {76, 72, 90}, {152, 148, 166}, {150, 100, 240}, {196, 166, 255}, {24, 22, 34}, {255, 255, 255}}   /* IRIS: iris violet */,
    {{21, 11, 48}, {56, 32, 116}, {170, 155, 208}, {255, 90, 200}, {140, 90, 255}, {11, 5, 24}, {244, 238, 255}}   /* GLUCKSMAN: deep violet and UV pink */,
    {{60, 12, 48}, {130, 36, 110}, {240, 170, 225}, {255, 0, 170}, {255, 70, 215}, {44, 0, 34}, {255, 232, 250}}   /* MAXENCE: a very loud pink, neon on plum */,
    {{66, 82, 100}, {96, 110, 128}, {138, 150, 164}, {255, 136, 56}, {84, 200, 240}, {44, 57, 72}, {236, 240, 244}}   /* RIBBON (K-GLITCH 0.2b): cyan values, an orange accent, slate cards on a slate night */};
static uint32_t skin_idx(void) { return (set.skin & 0x7Fu) % NSKINS; }
static uint32_t skin_old(uint32_t s) { return s < 18u ? SKIN_OLD[s] : SKIN_LOAF_DEFAULT; }
static uint16_t skin_block = RGB(24, 24, 30), skin_hi = RGB(255, 132, 112), skin_white = 0xFFFFu;
static uint16_t skin_bg;                 /* (K-GLITCH 0.2b: the ground, black but RIBBON's slate: C_BLACK) */
#define RIB (skin_idx() == SKIN_RIBBON)   /* (RIBBON's shapes: pills, rounded cards) */
/* LOAF BREAKER: SERATO's colours for a wave's column: its lows, mids and highs (0..255 each) as one hue, red (the lows)
 * through yellow, green (the mids), cyan to blue (the highs), at full saturation, br bright (0..255) */
static uint16_t serato_col(uint32_t lo, uint32_t mi, uint32_t hi, uint32_t br)
{
    uint32_t a = 3u * lo, b = mi, c = hi, s = a + b + c, h, r, g, bl;   /* (the lows weighed three times: a loud
                                          * kick, its three bands near full, orange, its tail red; a snare yellow-green,
                                          * a hat blue) */
    if (!s)
        return RGB(0, 0, 0);
    h = (b * 120u + c * 240u) / s;       /* (0 red .. 120 green .. 240 blue) */
    if (h < 60u) {
        r = 255u;
        g = h * 255u / 60u;
        bl = 0u;
    } else if (h < 120u) {
        r = (120u - h) * 255u / 60u;
        g = 255u;
        bl = 0u;
    } else if (h < 180u) {
        r = 0u;
        g = 255u;
        bl = (h - 120u) * 255u / 60u;
    } else {
        r = 0u;
        g = (240u - h) * 255u / 60u;
        bl = 255u;
    }
    return RGB(r * br / 255u, g * br / 255u, bl * br / 255u);
}
/* LOAF BREAKER (0.9.70, FLEXOR: "rajoute aussi 3 band et blue aux waveform classiques"): WAVE TYPE (set.wstyle, SYSTEM ›
 * DISPLAY K2): RGB (rekordbox's, as ever; REKORDBOX until 0.9.69), SERATO, 3 BAND (rekordbox's: the lows blue behind, the
 * mids orange, the highs white in front), BLUE (rekordbox's classic: blue, lighter where there are highs). wave_col draws
 * one column of a coloured wave in it: the CDJ view, the kick's and the drums' pictures, the strip (strip_style) */
enum { WS_RGB, WS_SERATO, WS_3BAND, WS_BLUE, WS_N };
static const char *const WS_NAME[WS_N] = {"RGB", "SERATO", "3 BAND", "BLUE"};   /* (RGB: RIBBON's colours with that skin) */
static uint32_t ws_style(void) { return set.wstyle < WS_N ? set.wstyle : WS_RGB; }
static void wave_col(int32_t x, int32_t mid, int32_t h, uint32_t lo, uint32_t mi, uint32_t hi, uint32_t br, uint32_t st,
                     int sq)                /* (sq: RGB's bands squared, saturated, as the kick's and the drums' pictures) */
{
    uint32_t m = lo > mi ? lo : mi;
    m = hi > m ? hi : m;
    if (!m || h <= 0)
        return;
    if (st == WS_RGB && RIB) {           /* (K-GLITCH 0.2b, RIBBON: the bands in the skin's colours, the lows orange (the
                                          * kicks), the mids cyan (the snares), the highs white (the hats), mixed by their
                                          * share; flat, one colour a column) */
        uint32_t wl = lo * 255u / m, wm = mi * 255u / m, wh = hi * 255u / m, s, r, g, b;
        wl = wl * wl * wl / 65025u, wm = wm * wm * wm / 65025u, wh = wh * wh * wh / 65025u;   /* (cubed: the strongest band wins) */
        s = wl + wm + wh + 1u;
        r = (wl * 255u + wm * 40u + wh * 236u) / s, g = (wl * 120u + wm * 210u + wh * 240u) / s, b = (wl * 30u + wm * 255u + wh * 250u) / s;
        cv_rect(x, mid - h, 1, 2 * h + 1, RGB(r * br / 255u, g * br / 255u, b * br / 255u));   /* (flat: one colour) */
    } else if (st == WS_SERATO) {
        cv_rect(x, mid - h, 1, 2 * h + 1, serato_col(lo, mi, hi, br));
    } else if (st == WS_3BAND) {         /* (each band as tall as it is against the strongest, the lows behind) */
        static const uint8_t C[3][3] = {{36, 104, 255}, {255, 150, 40}, {245, 245, 245}};
        static const uint8_t K[3] = {150, 95, 45};   /* (the lows the widest, as rekordbox draws them) */
        uint32_t v[3] = {lo, mi, hi}, j;
        for (j = 0; j < 3u; j++) {
            int32_t hh = (int32_t)((uint32_t)h * v[j] * K[j] / (100u * m));
            if (hh > h)
                hh = h;
            if (hh > 0 || (j == 0u && v[j]))
                cv_rect(x, mid - hh, 1, 2 * hh + 1, RGB(C[j][0] * br / 255u, C[j][1] * br / 255u, C[j][2] * br / 255u));
        }
    } else if (st == WS_BLUE) {          /* (dark blue, to pale blue-white with the highs) */
        uint32_t s = lo + mi + hi, r = s ? hi * 560u / s : 0u;
        r = r > 255u ? 255u : r;
        cv_rect(x, mid - h, 1, 2 * h + 1, RGB((20u + 150u * r / 255u) * br / 255u, (70u + 155u * r / 255u) * br / 255u,
                                              (210u + 45u * r / 255u) * br / 255u));
    } else {
        uint32_t r = lo * 255u / m, g = mi * 255u / m, b = hi * 255u / m;
        if (sq) {
            r = r * r / 255u;
            g = g * g / 255u;
            b = b * b / 255u;
        }
        cv_rect(x, mid - h, 1, 2 * h + 1, RGB(r * br / 255u, g * br / 255u, b * br / 255u));
    }
}
/* (0.9.70, FLEXOR: "la petite waveform grise ... stylisée") the strip over the master's wave (screen.c draw_wave): RGB
 * (the start), 3 BAND, BLUE, SERATO, or GREY (as it always was); SYSTEM › DISPLAY's PRESETS. In set.bta_codec (no
 * Bluetooth: its old byte), 0xC0 | it; untagged RGB */
enum { SS_RGB, SS_3BAND, SS_BLUE, SS_SERATO, SS_GREY, SS_N };
static const char *const SS_NAME[SS_N] = {"RGB", "3 BAND", "BLUE", "SERATO", "GREY"};
static const uint8_t SS_WS[SS_N] = {WS_RGB, WS_3BAND, WS_BLUE, WS_SERATO, WS_RGB};
static uint32_t strip_style(void)
{
    return (set.bta_codec & 0xF0u) == 0xC0u && (set.bta_codec & 15u) < SS_N ? set.bta_codec & 15u : SS_RGB;
}
static void strip_put(uint32_t s) { set.bta_codec = (uint8_t)(0xC0u | s % SS_N); }
static uint16_t skin_rgb(const uint8_t *c, uint32_t pct)   /* a colour at pct % of its brightness */
{
    return RGB(c[0] * pct / 100u, c[1] * pct / 100u, c[2] * pct / 100u);
}
static void skin_apply(uint32_t s)
{
    const uint8_t (*c)[3] = SKIN_RGB[s % NSKINS];
    uint32_t k;
    for (k = 0; k < 5u; k++)             /* fm1-base's five (C_LINE .. C_HI) */
        pal[k] = skin_rgb(c[k], 100u);
    skin_block = skin_rgb(c[5], 100u);
    skin_hi = skin_rgb(c[4], 100u);
    skin_white = skin_rgb(c[6], 100u);
    skin_bg = s % NSKINS == SKIN_RIBBON ? RGB(20, 27, 35) : 0u;
}
/* K-GLITCH 0.2b: a rounded box (RIBBON's cards and pills) */
static void rbox(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    cv_rect(x + 2, y, w - 4, h, c);
    cv_rect(x + 1, y + 1, w - 2, h - 2, c);
    cv_rect(x, y + 2, w, h - 4, c);
}
/* RIBBON CLASSIC (K-GLITCH 0.2b): every page's content (rows RIB_Y0..RIB_Y1, under the tabs, over the black keys) on one
 * rounded card, a line round it: in a full-width strip about to be sent, the ground's pixels there become the card's */
#define RIB_Y0 36u
#define RIB_Y1 190u
static void rib_frame(uint32_t x, uint32_t y)
{
    uint32_t r, i, Y;
    uint16_t bg, card, line;
    if (!RIB || x || cv_w != 240u)
        return;
    bg = swap16(skin_bg);
    card = swap16(RGB(29, 39, 50));
    line = swap16(pal[0]);
    for (r = 0; r < cv_h; r++) {
        uint16_t *p = cv_px + r * 240u;
        uint32_t in = 0u;
        Y = y + r;
        if (Y < RIB_Y0 || Y > RIB_Y1)
            continue;
        if (Y == RIB_Y0 || Y == RIB_Y1) {          /* the top and bottom lines, the corners cut */
            for (i = 6u; i < 234u; i++)
                if (p[i] == bg) p[i] = line;
            continue;
        }
        in = Y == RIB_Y0 + 1u || Y == RIB_Y1 - 1u ? 2u : Y == RIB_Y0 + 2u || Y == RIB_Y1 - 2u ? 1u : 0u;
        if (p[3u + in] == bg) p[3u + in] = line;
        if (p[236u - in] == bg) p[236u - in] = line;
        for (i = 4u + in; i < 236u - in; i++)
            if (p[i] == bg) p[i] = card;
    }
}
#undef C_BLACK                           /* (the ground is the skin's: RIBBON's slate) */
#define C_BLACK skin_bg
#undef C_WHITE                           /* (the app's white is the skin's: put back after the UI, app.c's end) */
#define C_WHITE skin_white

enum { PG_HOME, PG_GEN, PG_FX, PG_GLOBAL, PG_KICK, PG_MIX, PG_SEQ, PG_PERC, PG_TRACK, PG_OVL, PG_KEQ, PG_HSET, PG_SPLIT, PG_CHAOS, PG_KG, NPAGES };   /* (K-GLITCH 0.2b: PG_KG, SOUNDS' GLITCH page, ui/kgchop.c) */   /* (PG_SPLIT: HOME's third part, 0.9.71) */
#define HOME_PG(p) ((p) == PG_HOME || (p) == PG_GEN || (p) == PG_SPLIT || (p) == PG_CHAOS)   /* (0.9.75: CHAOS) */   /* (HOME's parts: PERFORM, GENERATOR, SPLIT) */
/* (LOAF BREAKER, 0.9.2, FLEXOR: PG_HSET, HOME's SETTINGS after PERFORM and GENERATOR) */
                                         /* (LOAF BREAKER: PG_KEQ, HOME's third part, the KICK EQ; PG_TRACK, ENV's
                                          * TRACKS, PLAY, DROP where TAP TEMPO was) */
/* LOAF BREAKER (11 Nov 2026): the black keys' functions' names (app.c BKF_*), as the strip shows them */
static const char *const BKF_NAME[BKF_N] = {"R8", "R16", "R32", "REV", "FRZ", "HLF", "VNL", "GAT", "RND", "FLT", "DUB",
                                            "ARP", "KIL", "DST", "SUB", "DRV", "X2", "X4", "TRI", "SDR", "KBR", "CHS"};
static const char *const BKF_LONG[BKF_N] = {"ROLL 1/8", "ROLL 1/16", "ROLL 1/32", "REVERSE", "FREEZE", "HALF SPEED",
                                            "VINYL BREAK", "GATE", "RANDOM", "FILTER", "DUB", "ARP (OVERLAY)",
                                            "KICKS OFF", "DISTO", "SUB FX", "DRIVE", "KICKS X2",
                                            "KICKS X4", "KICK TRIPLETS", "SUB DRIVE", "KILL BREAK", "CHAOS"};
static const char *const BKP_NAME[BKP_N] = {"CHOPPAH", "FLEXOR LAYOUT", "CUSTOM"};
static const uint8_t WK[16] = PLAT_WHITE_KEYS, BK[11] = PLAT_BLACK_KEYS;
/* LOAF BREAKER: what the TUTORIAL (tutor.c) watches, counted where it happens: white keys pressed, black keys pressed
 * (a bit each), K1-K4 turned on the KICK menu, SEL + K4 turned, drum steps set on SEQ, SAVEs */
/* LOAF BREAKER (0.216, FLEXOR): EDIT ZOOM, how much time EDIT's pictures show (SYSTEM › EFFECTS, PRESETS) */
#define EZ_N 7u
static const uint16_t EZ_MS[EZ_N] = {0u, 250u, 500u, 750u, 1000u, 1500u, 2000u};   /* (0.257, FLEXOR: 750 MS) */
static const char *const EZ_NAME[EZ_N] = {"AUTO", "250 MS", "500 MS", "750 MS", "1 S", "1.5 S", "2 S"};
#define EZ_TAG 0x40u                     /* (set.ezoom: EZ_TAG | the index since 0.257; an older save's 0..5, 750 MS not
                                          * there yet: 3.. one further on) */
static uint32_t ez_idx(void)
{
    uint32_t v = set.ezoom;
    if (v & EZ_TAG)
        return (v & 0x3Fu) % EZ_N;
    v %= 6u;
    return v >= 3u ? v + 1u : v;
}
static uint32_t tut_wk, tut_pads, tut_padn, tut_kturn, tut_sk4, tut_lane, tut_saves, tut_home, tut_kpre, tut_ovl;
static uint8_t tut_heard, tut_drs;   /* (LOAF BREAKER, 0.252: the drums played on keys 1-4 since the last knob turn; and
                                          * since the step's start) */
/* LOAF BREAKER (11 Nov 2026): the keys of a function (BKF_*) in the layout in use (app.c bk_fn), a bit each (none:
 * 0). The pads' code asks PAD_BIT(pad) */
static uint32_t bkf_mask(uint32_t f)
{
    uint32_t j, m = 0u;
    for (j = 0; j < 11u; j++)
        if (bk_fn(j) == f)
            m |= 1u << BK[j];
    if ((rec_key_on() || bk_sel_dn >> 11 & 1u) && bk_fn(11u) == f)   /* (0.9.58: the REC key's; 0.9.70 with SEL its SEL function) */
        m |= REC_KEY;
    return m;
}
#define PAD_BIT(i) bkf_mask((uint32_t)(i))

static struct {
    uint32_t keys, btn;
    uint8_t page, fx_focus;              /* fx_focus: the FX chain's slot the FX page tunes (0..3) */
    char msg[2][24];
    uint32_t msg_until;
    uint32_t tap_t;                      /* SEL went down then: a tap if it comes up alone */
    uint8_t tap_pend, tap_combo;         /* SEL down as a tap; something else touched while it was held */
    uint32_t oct_t, oct_rep;             /* OCT- / OCT+ held: repeats that speed up */
    int32_t last_step, last_slice;
    uint32_t last_pads, last_keys, last_hdr;
    uint32_t kick_t;                     /* LOAF BREAKER TEST: when the kick last hit (the header's flash) */
    uint32_t kick_seen;
    uint32_t kscope_drawn;
    uint32_t aud_t;                      /* LOAF BREAKER: a drum played on its key (the classic view, the drums' pages):
                                          * when, which */
    uint8_t aud_d;
    uint8_t opage, bk_sel;               /* LOAF BREAKER: the ARP page's tab (0 ARP EFFECTS, 1 BLACK KEYS, 2 MORE;
                                          * BEGINNER: the first only), the key BLACK KEYS edits (0..10) */
    uint8_t pad_more;                    /* LOAF BREAKER: the ARP key's knobs: 0 MAIN, 1 MORE (FX while it is held) */
    uint8_t pbig;                        /* LOAF BREAKER: the drums' page in its big view (SELECT from the KICK menu's big
                                          * view); 0: the classic one (SEL + EDIT) */
    uint32_t intro_t0;                   /* the start screen: when it began (0: over) */
    uint8_t intro, intro_drawn;          /* showing it; blocks drawn so far */
    uint8_t touched;                     /* a key, button or encoder moved since app_frame last looked */
    uint32_t panel_drawn;                /* the GLOBAL panel's, the MB COMP meter's last frame */
    int8_t pad_focus;                    /* the pad held last (its animation, K1-K4 tune it); -1 none */
    int8_t key_focus;                    /* the white key held that loops (its view, K1-K4 tune it); -1 none */
    uint8_t key_layer;                   /* K1-K4 on a key held: 0 its loop, 1 its slice's effect (FX flips) */
    uint8_t key_turned;                  /* a knob turned while it is held: its view at once */
    uint32_t key_since;                  /* when it was pressed */
    int16_t shown;                       /* the strip shows: -1 the blocks, 0..10 a pad, 100 + k white key k */
    uint8_t gpage;                       /* the GLOBAL page's part: 0 MIDI and Bluetooth, 1 pitch, 2 BT audio */
    uint8_t bta_sel, bta_booted;         /* BT AUDIO: the device chosen in its list; the boot's reconnect done */
    uint8_t kpage, klayer, kpreset;      /* the KICK page: its sub-page (KP_*), the layer it edits, the preset */
    uint8_t mpage;                       /* the MIXER's half: 0 LEVELS (break, kick, perc, FX), 1 GROOVE */
    uint8_t seq_lane, seq_step;          /* the SEQ page: the lane (SL_*) and the break step (0..31) it edits */
    uint8_t pvoice, ppage, ppreset[PERC_VOICES];   /* the SNARE / HAT page: its voice, sub-page, presets */
    uint8_t einst;                       /* EDIT, the drums' design: DR_*: the kick (PG_KICK), the snare, the hat, the
                                          * clap (PG_PERC) */
    uint8_t rec;                         /* REC: white keys and MIDI notes write into the pattern as it plays */
    uint8_t help;                        /* SEL pressed alone: the SEL combinations on the strip, until the next press */
    uint32_t help_t;                     /* when the card opened */
    uint32_t sel_t;                      /* LOAF BREAKER (0.258): when SEL went down (held a moment: the foot its knobs) */
    uint8_t sel_foot;                    /* (the foot showing SEL + K1-K4's, drawn so) */
    uint8_t drawn_page;                  /* the page last drawn (TAP takes the whole screen: leaving it redraws all) */
    uint32_t idle_t;                     /* the last key, button or knob touched (the demo: demo.c) */
    uint32_t lobo_t;                     /* RESET ALL done: the lobotomy screen, since then (0: not showing) */
    uint8_t home_hold, home_combo;       /* HOME held on its page (up alone: its other part); PLAY pressed meanwhile */
    uint8_t cdj, cdj_skip, blocks;       /* LOAF BREAKER: the CDJ view up (PERFORM / GENERATOR's default); this HOME
                                          * press done with it; the slice blocks instead (HOME held: either) */
    uint32_t home_t;                     /* (when HOME went down) */
    uint8_t fx_hold, fx_turned;          /* FX held on the FX page (up alone: the slot's next part); a knob turned meanwhile */
    uint8_t fx_page;                     /* the focused slot's part: MAIN, MORE, MOD (an MB COMP: MBP_*) */
    float mb_hold[3];                    /* its bands' reduction peaks held (dB), and since when */
    uint32_t mb_hold_t[3];
    uint32_t reset_t;                    /* GLOBAL's RESET ALL: when it was last turned */
    uint32_t drop_t, build_said;         /* LOAF BREAKER: the DROP's flash since (0: not); the build's last word */
    uint8_t drop_seen;                   /* the drops shown (drop_n's) */
    uint8_t drop_pat;                    /* the pattern the drop moves to (0xFF: none) */
    uint8_t drop_early, drop_asked;      /* (crossbreed) the drop asked early; its break asked for */
    uint8_t note_key, note_turned;
    uint8_t fx_open;
    uint8_t keq_part;                    /* (the KICK EQ page: 0 its bands, 1 their shape) */                     /* (FX pressed off its page: the page at its release, if not held) */       /* (crossbreed) SEQ, a kick lane: the key held + 1 (0 none); K1 turned since */
    uint8_t tpage;                       /* LOAF BREAKER: ENV's part: 0 TRACKS, 1 PLAY, 2 DROP, 3 KNOB */
    float radio;                         /* LOAF BREAKER: ENV PLAY's dial: where its needle is (eased to the track) */
    uint8_t kil_focus;                   /* LOAF BREAKER: KIL held (its K1: LATCH) */
    uint8_t x2_focus;                    /* (0.9.48: X2 held, its K1: LATCH; 0.9.55: 1 X2, 2 X4, 3 TRI) */
    uint8_t seq_brk;                     /* LOAF BREAKER (0.234): EXPERT's SEQ on its BREAK entry (above the lanes): K4
                                          * the break's rows shown (set.brkv); SEQ_BRK */
    uint8_t seq_manual;                  /* LOAF BREAKER: SEQ's lane picked by hand while playing: the bar playing not
                                          * followed until the next stop */
    uint8_t meter_on;                    /* (TEST) SYSTEM › MIDI K3 METER (EXPERT): the load, once a second in the header */
    uint8_t ksect, kpos[7], kks;         /* (kks: KNOBS' K SET, the knob 0..3; LOAF BREAKER: six sections, LAB) */
    uint8_t kview;                       /* (LOAF BREAKER: the KICK menu's picture: 0 RGB, 1 WAVE; EDIT held, then CLASSIC) */
    uint8_t kclassic, edit_hold;         /* (EDIT held on the KICK menu: the view as it was, the drums' shapes; EDIT
                                          * down: when, and whether it has done its hold) */
    uint32_t edit_t;              /* LOAF BREAKER: the KICK menu's section (SOUND SYNTH DIRT TONE), each one's part */
    uint8_t env_hold;                    /* LOAF BREAKER: ENV down on TRACKS: 1 waiting (let go: the next part), 2 held
                                          * (a gap made) */
    uint32_t env_t;
    uint8_t rst_sel;                     /* LOAF BREAKER: ENV's RESET: what it resets (RST_*) */
    uint32_t rst_arm;                    /* (the last click of K2-K4: when; RST_TURNS clicks, each within 2 s, the reset) */
    uint8_t rst_n;                       /* (the clicks so far) */
    uint32_t rst_ask;                    /* (LOAF BREAKER, 0.239: the clicks done, GLO asked for: since when, |1; 0 none) */
    uint32_t rst_done;                   /* (LOAF BREAKER: a reset done, when: the RESET page's knobs do nothing for
                                          * RST_COOL ms after, the clicks past the last one lost, DONE left to read) */
    uint8_t lfo_hold;                    /* LOAF BREAKER: LFO down (1), a level turned with it (2): no page then */
    uint8_t sfx_cur;                     /* LOAF BREAKER: GLO EFFECTS: the effect K1 points at (SFX_CRT..SFX_8BIT) */
    uint8_t sel_lock, sel_raw;           /* LOAF BREAKER: SEL locked down (SEL x2; pressed again: free); SEL's
                                          * own state last frame */
    uint8_t knfn;                        /* KNOB's function: 0 DUCK (K1), 1 TERROR 2, 2 COMP, 3 TERROR (K4) */
    uint8_t fx_main, kfx_last;           /* (KICK FX: the FX page on slot 4 or 5; the chain's slot kept meanwhile, and
                                          * the kick's last) */
    uint8_t trk_cur, trk_from;           /* TRACKS' item edited; PLAY's first item */
    uint8_t trk_on;                      /* a track playing (2: starting, not yet heard) */
    uint8_t trk_pos, trk_loops, trk_cnt; /* its item, the loops it played, this loop counted */
    uint8_t trk_drop, trk_stop;          /* the item a drop of it goes to (0xFF none); the stop at the loop's end */
    int16_t trk_prev;                    /* the step seen last (a wrap: a new loop) */
    uint8_t sel_clicks;                  /* SEL's clicks in a row (three: the build) */
    uint32_t sel_click_t;
    uint8_t reset_n;                     /* GLOUPI BREAKS: its turns so far, each within 3 s of the last (it wants 15) */
    uint32_t mix_drawn;                  /* its faders' meters: when last drawn */
    int16_t meter_x;                     /* the header's level meter: where (0: not shown now, a message is) */
    uint32_t meter_drawn;
    uint32_t pad_since, fx_drawn;        /* when it was pressed; the animation's last frame */
    float reel;                          /* the tape stop's reels, turns */
    uint8_t redraw;                      /* RG_* bits: the regions to draw again */
} ui;
/* the white keys play slices (or, on the SEQ, KICK and SNARE / HAT pages, steps) */
static void page_say(void);              /* the header: the page's button and part, as its tabs (main.c) */
#define SEQ_BRK (ui.seq_brk && !set.novice)
static int slices_page(void) { return (ui.page != PG_SEQ || SEQ_BRK) && ui.page != PG_TRACK && ui.page != PG_OVL &&
                                         !(ui.page == PG_GLOBAL && ui.gpage == 3u); }   /* (GLO EFFECTS: keys 1-7) */   /* (ARP: overlays) */
enum { RG_HEAD = 1, RG_WAVE = 2, RG_BLOCKS = 4, RG_FOOT = 8, RG_ALL = 15, RG_FX = 16 };

static void text_cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static void int_text(int32_t v, char *b)
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void tenths_text(int32_t v, char *b)   /* 1700 -> "170.0" */
{
    char t[12];
    int_text(v / 10, b);
    t[0] = '.';
    t[1] = (char)('0' + (v < 0 ? -v : v) % 10);
    t[2] = 0;
    text_cat(b, t);
}

void app_say(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1300u;
    ui.redraw |= 1u;               /* RG_HEAD: the message */
}

static void say_int(const char *what, int32_t v, const char *unit)
{
    char t[24];
    int_text(v, t);
    text_cat(t, unit);
    app_say(what, t);
}

static float knob_f(float v, int32_t d, float step, float lo, float hi)
{
    v += (float)d * step;
    return v < lo ? lo : v > hi ? hi : v;
}

/* ------------------------------------------------------------- the FX chain --- */

static void hz_text(float hz, char *t)   /* 340 -> "340HZ", 1234 -> "1.2KHZ" (LOAF BREAKER, 0.217: the unit always) */
{
    t[0] = 0;
    if (hz >= 1000.0f) {
        tenths_text((int32_t)(hz / 100.0f + 0.5f), t);
        text_cat(t, "KHZ");
    } else {
        int_text((int32_t)(hz + 0.5f), t);
        text_cat(t, "HZ");
    }
}

static void hundredths_text(float v, char *t)   /* 0.25 -> "0.25" (0..99.99) */
{
    int32_t c = (int32_t)(v * 100.0f + 0.5f);
    char d[4];
    int_text(c / 100, t);
    text_cat(t, ".");
    d[0] = (char)('0' + c / 10 % 10);
    d[1] = (char)('0' + c % 10);
    d[2] = 0;
    text_cat(t, d);
}

static void pct_text(float v, char *t)   /* 0.42 -> "42%" */
{
    int_text((int32_t)(v * 100.0f + (v < 0.0f ? -0.5f : 0.5f)), t);
    text_cat(t, "%");
}
static void signed_tenths(float v, const char *unit, char *t)   /* -1.5 -> "-1.5ST" */
{
    int32_t x = (int32_t)(v * 10.0f + (v < 0.0f ? -0.5f : 0.5f));
    t[0] = x > 0 ? '+' : 0;
    t[1] = 0;
    tenths_text(x, t + (x > 0));
    if (x < 0 && x > -10) {              /* tenths_text drops the sign of -0.x */
        t[0] = '-';
        tenths_text(-x, t + 1);
    }
    text_cat(t, unit);
}
static void times_text(float v, char *t) /* 0.5 -> "x0.50" */
{
    int32_t x = (int32_t)(v * 100.0f + 0.5f);
    char d[8];
    t[0] = 'x';
    int_text(x / 100, t + 1);
    text_cat(t, ".");
    d[0] = (char)('0' + x / 10 % 10);
    d[1] = (char)('0' + x % 10);
    d[2] = 0;
    text_cat(t, d);
}

/* ------------------------------------------------------------- start screen --- */
#define C_BLOCK skin_block              /* a slice block's ground (the skin's) */
#define C_BLOCK_HI skin_hi                /* the block sounding */

/* big text: a font at twice its size, each of its pixels a 2 x 2 block (blended as cv_text blends). It stands in for
 * a 31-pixel font: that one costs 20 KB of flash, which Bluetooth audio needed */
static int32_t text2_w(const felucca_font_t *f, const char *s) { return 2 * text_w(f, s); }
static int32_t cv_textn(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, uint32_t n);
static int32_t cv_text2(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    return cv_textn(x, y, f, s, c, 2u);
}
/* a font at n times its size (TAP's tempo: 3) */
static int32_t cv_textn(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c, uint32_t n)
{
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u;
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w = f->bw[gi], bpr = (w + 1u) / 2u;
        const uint8_t *gd = f->data + f->off[gi];
#ifdef FONT_TRIM                         /* (LOAF BREAKER: the fonts stored trimmed, fm1.toml font_trim: the glyph its lit
                                          * box, after its left, top and rows) */
        int32_t ox = gd[0], oy = gd[1];
        uint32_t rows = gd[2];
        gd += 3;
#else
        int32_t ox = 0, oy = 0;
        uint32_t rows = f->h;
#endif
        for (gy = 0; gy < rows; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t a = gd[gy * bpr + gx / 2u], k;
                a = (gx & 1u) ? (a & 15u) : (a >> 4);
                if (!a)
                    continue;
                for (k = 0; k < n * n; k++) {
                    int32_t px = x + (int32_t)n * ((int32_t)gx + ox - f->pad) + (int32_t)(k % n),
                            py = y + (int32_t)n * ((int32_t)gy + oy) + (int32_t)(k / n) + cv_oy;
                    uint32_t at, bg;
                    if ((uint32_t)px >= cv_w || py < cv_y0 || py >= cv_y1)
                        continue;
                    at = (uint32_t)py * cv_w + (uint32_t)px;
                    bg = swap16(cv_px[at]);
                    cv_px[at] = swap16((((r * a + (bg >> 11) * (15u - a)) / 15u) << 11) |
                                       (((g * a + ((bg >> 5) & 63u) * (15u - a)) / 15u) << 5) |
                                       ((b * a + (bg & 31u) * (15u - a)) / 15u));
                }
            }
        x += (int32_t)n * f->adv[gi];
    }
    return x;
}

/* LOAF BREAKER (FLEXOR, 11 Nov 2026, 0.229): a knob BEGINNER leaves out (drawn blank), turned: said where it is */
static void easy_no(void) { app_say("EXPERT MODE", "SETUP K3: EXPERT"); }
