/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the start screen, the header, the break's waveform and the slice blocks. (Part of app.c's unity:
 * included there in order.) */

#define INTRO_STEP 55u                   /* ms between two blocks dropping in */
#define INTRO_HOLD 900u                  /* ms the finished screen stays before the slicer opens */

/* the mark: sixteen bars like a break's hits, two of them chopped out of line */
/* the break in the header: its name, a split's two ("AMEN10 + FOCUZ": the first word of each, 0.9.71) */
static const char *hdr_brk_name(void)
{
    static char t[24];
    const char *n;
    uint32_t k = 0u, j;
    if (!split_on())
        return break_name(set.brk);
    for (n = break_name(set.brk), j = 0; n[j] && n[j] != ' ' && k < 8u; j++)
        t[k++] = n[j];
    t[k++] = '+';
    for (n = break_name(split_b_of(set.brk)), j = 0; n[j] && n[j] != ' ' && k < 19u; j++)
        t[k++] = n[j];
    t[k] = 0;
    return t;
}
static const uint8_t MARK_H[16] = {30, 12, 26, 10, 34, 14, 11, 20, 12, 22, 28, 24, 34, 10, 13, 22};
#include "pp_wave.h"                     /* (LOAF BREAKER: PRESS PLAY's picture, tools/make_pp_wave.py) */
static void mark(int32_t y0, uint32_t shown, const uint8_t (*px)[BLK_W])
{
    uint32_t k, c;
    for (k = 0; k < 16u && k < shown; k++) {
        int32_t x = 8 + (int32_t)k * 14, lift = (k == 5u || k == 11u) ? -10 : 0;
        uint16_t col = (k == 5u || k == 11u) ? C_WHITE : C_BLOCK_HI;
        if (px) {                        /* the loaded break: each slice's own shape */
            for (c = 0; c < 12u; c++) {
                int32_t h = px[k][c * BLK_W / 12u] * 2 + 1;
                cv_rect(x + (int32_t)c, y0 + lift + 20 - h / 2, 1, h, col);
            }
        } else                           /* the splash, before any break is in: the drawn mark */
            cv_rect(x, y0 + lift + 20 - MARK_H[k] / 2, 12, MARK_H[k], col);
    }
}

/* the platform's own screens' faces (APP_SYS_TEXT, device/sys_text.c): the body FONT_XS, a heading FONT_B, the big
 * one FONT_B doubled (the device's: the simulator draws none of them) */
#ifndef PLAT_HOST
static int32_t app_sys_h(int face)
{
    return face == SYS_BIG ? 2 * (int32_t)FONT_B.h : face == SYS_B ? FONT_B.h : FONT_XS.h;
}
static int32_t app_sys_w(int face, const char *s)
{
    return face == SYS_BIG ? text2_w(&FONT_B, s) : text_w(face == SYS_B ? &FONT_B : &FONT_XS, s);
}
static void app_sys_text(int32_t x, int32_t y, int face, const char *s, uint16_t c)
{
    if (face == SYS_BIG)
        cv_text2(x, y, &FONT_B, s, c);
    else
        cv_text(x, y, face == SYS_B ? &FONT_B : &FONT_XS, s, c);
}
#endif

static void title(int32_t y)             /* the name and the tagline, centred, into the current strip */
{
    cv_text2((240 - text2_w(&FONT_B, APP_NAME)) / 2, y, &FONT_B, APP_NAME, C_WHITE);
#if CH_TAGLINE
    cv_text((240 - text_w(&FONT_XS, "AMEN CHOPPER  FOR THE FM-1")) / 2, y + 36, &FONT_XS,
            "AMEN CHOPPER  FOR THE FM-1", C_HI);
#endif
}

/* the version, as the screens show it: "V0.7" (a release), "DEV" (any other build) */
static void version_text(char *t)
{
    t[0] = 0;                            /* (GLOUPI BREAKS: the number alone, no "V") */
    text_cat(t, CH_VERSION);
}


/* GLOUPI BREAKS: a grey picture (tools/make_egg.py: a byte a run, bits 0-3 the grey, 4-7 the length less one) at
 * x0, y0 on the screen, w x h, in strips of 60 rows, each the screen's width (its sides black) */
static int32_t gl_amt;                   /* LOAF BREAKER TEST: the start-up glitch: how far rows jump (0: none), its seed */
static uint32_t gl_seed;
static int32_t gl_band = -1;             /* (LOAF BREAKER: VHS: the tracking band's top row, -1 none) */
static int32_t gl_shift(int32_t row)     /* a band of rows: shifted, or not (a third of the bands) */
{
    uint32_t h = ((uint32_t)row / 5u + gl_seed) * 2654435761u;
    if (!gl_amt || (h >> 28) % 3u)
        return 0;
    return (int32_t)((h >> 8) % (uint32_t)(2 * gl_amt + 1)) - gl_amt;
}
static void grey_draw(const uint8_t *rle, uint32_t len, const uint16_t *pal, int32_t w, int32_t h, int32_t x0,
                      int32_t y0)
{
    int32_t r, sh;
    for (r = 0; r < h; r += sh) {
        uint32_t i;
        int32_t x = 0, row = 0;
        sh = h - r < 60 ? h - r : 60;
        cv_begin(240, (uint32_t)sh, C_BLACK);
        for (i = 0; i < len && row < r + sh; i++) {
            uint32_t b = rle[i], c = b & 15u;
            int32_t n = (int32_t)(b >> 4) + 1;
            if (c && row >= r) {
                int32_t g = gl_shift(row);   /* (the glitch: the band shifted, its light greyed) */
                cv_rect(x0 + x + g, row - r, n, 1, g ? (g & 1 ? RGB(150, 150, 156) : pal[c]) : pal[c]);
            }
            x += n;
            if (x >= w) {
                x = 0;
                row++;
            }
        }
        if (gl_amt) {                    /* (LOAF BREAKER: VHS: the tracking band rolling down, a smear of grey
                                          * noise, the row above it dark; specks here and there) */
            int32_t k;
            for (k = 0; k < sh; k++) {
                int32_t row = r + k, in = row >= gl_band && row < gl_band + 7;
                uint32_t hsh = ((uint32_t)row * 2246822519u) ^ (gl_seed * 3266489917u), m;
                if (row == gl_band - 1)
                    cv_rect(0, k, 240, 1, C_BLACK);
                for (m = 0; m < (in ? 26u : 2u); m++) {
                    uint32_t q = (hsh += 0x9E3779B9u) * 2654435761u, lv = 90u + (q >> 25);
                    cv_rect((int32_t)((q >> 8) % 240u), k, in ? 2 + (int32_t)((q >> 4) & 7u) : 1, 1,
                            RGB(lv, lv, lv + 6u));
                }
            }
        }
        cv_blit(0, (uint32_t)(y0 + r));
    }
}



/* fm1-base's first screen at power-on, while the breaks are decoded (APP_SPLASH): the mark and the name (LOAF
 * BREAKER: its picture, nearly the whole screen, the version at the top right; held 1.5 s, app_frame) */
static uint32_t splash_t;                /* when it was drawn (0: gone) */
static void splash_glitch(uint32_t age)  /* LOAF BREAKER TEST: the start-up picture, its rows jumping, settling by 700 ms */
{
#if CH_LOGO
    gl_amt = age < 700u ? (int32_t)((700u - age) * 24u / 700u) + 1 : 0;
    gl_seed = age / 40u;
    sfx_splash_d = (uint8_t)(2 + gl_amt / 4);   /* (LOAF BREAKER: its red off, the more as it jumps) */
    gl_band = age < 700u ? (int32_t)(age * (uint32_t)SPLASH_H / 560u) - 8 : -1;   /* (rolling down the picture) */
    grey_draw(SPLASH_RLE, sizeof SPLASH_RLE, SPLASH_PAL, SPLASH_W, SPLASH_H, (240 - SPLASH_W) / 2, 240 - SPLASH_H);
    gl_amt = 0;
    gl_band = -1;
    sfx_splash_d = 0u;
#else
    (void)age;
#endif
}
void app_splash(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
#if CH_LOGO
    sfx_splash_d = 2u;                   /* (LOAF BREAKER: the picture's red two pixels off) */
    grey_draw(SPLASH_RLE, sizeof SPLASH_RLE, SPLASH_PAL, SPLASH_W, SPLASH_H, (240 - SPLASH_W) / 2, 240 - SPLASH_H);
    sfx_splash_d = 0u;
    {
        char t[24];                     /* (a TEST's "0.268 TEST 12" too) */
        version_text(t);
        cv_begin(240, 240 - SPLASH_H, C_BLACK);
        cv_text(234 - text_w(&FONT_T, t), 1, &FONT_T, t, C_GRAY);
        cv_blit(0, 0);
    }
    splash_t = plat_ms() | 1u;
#else
    cv_begin(240, 60, C_BLACK);
    mark(10, 16u, 0);
    cv_blit(0, 50);
    cv_begin(240, 56, C_BLACK);
    title(0);
    cv_blit(0, 118);
    {
        char t[24];
        version_text(t);
        cv_begin(240, 20, C_BLACK);
        cv_text((240 - text_w(&FONT_T, t)) / 2, 2, &FONT_T, t, C_GRAY);
        cv_blit(0, 210);
    }
#endif
}

static void intro_draw(uint32_t now)
{
    uint32_t shown = (now - ui.intro_t0) / INTRO_STEP + 1u;
    char t[32];
    if (shown > 16u)
        shown = 16u;
    if (!ui.intro_drawn)                 /* GLOUPI BREAKS: the splash's picture away first (this screen draws in bands) */
        lcd_fill(0, 0, 240, 240, C_BLACK);
    if (shown != ui.intro_drawn) {       /* LOAF BREAKER: a bar of APACHE with the kick on 1 and 9 through SUB DRIVE,
                                          * as the CDJ view draws it (pp_wave.h: the lows red, the mids green, the highs
                                          * blue), across the screen, nearly as tall as it goes, coming in a step at a
                                          * time, clean of the screen effects */
        uint32_t x, band;
        ui.intro_drawn = (uint8_t)shown;
        for (band = 0; band < 2u; band++) {   /* (y 38..110, two bands of 36 about y 74) */
            cv_begin(240, 36, C_BLACK);
            for (x = 0; x < shown * 15u && x < 240u; x++) {
                const uint8_t *v = PP_WAVE[x];
                uint32_t m = v[0] > v[1] ? v[0] : v[1], br = 110u + (uint32_t)v[3] * 145u / 255u;
                int32_t h = (int32_t)((uint32_t)v[3] * 32u / 255u);
                if (v[2] > m)
                    m = v[2];
                if (m && h)
                    cv_rect((int32_t)x, 36 - (int32_t)band * 36 - h, 1, 2 * h + 1,
                            RGB(v[0] * 255u / m * br / 255u, v[1] * 255u / m * br / 255u, v[2] * 255u / m * br / 255u));
            }
            sfx_splash_d = 255u;         /* (no screen effect on it: its colours clean) */
            cv_blit(0, 38u + band * 36u);
            sfx_splash_d = 0u;
        }
    }
    if (ui.redraw & RG_HEAD) {
        const brk_break_t *b = brk_get_break();
        cv_begin(240, 20, C_BLACK);      /* the version, at the top */
        version_text(t);
        cv_text((240 - text_w(&FONT_T, t)) / 2, 2, &FONT_T, t, C_GRAY);
        cv_blit(0, 16);
#if CH_LOGO                              /* LOAF BREAKER: its lettering */
        grey_draw(TITLE_RLE, sizeof TITLE_RLE, TITLE_PAL, TITLE_W, TITLE_H, (240 - TITLE_W) / 2, 114);
#else
        cv_begin(240, 56, C_BLACK);
        title(0);
        cv_blit(0, 118);
#endif
        cv_begin(240, 50, C_BLACK);
        t[0] = 0;
        text_cat(t, b ? b->name : "");
        cv_text((240 - text_w(&FONT_XS, t)) / 2, 2, &FONT_XS, t, C_GRAY);
        cv_text((240 - text_w(&FONT_T, "PRESS PLAY")) / 2, 20, &FONT_T, "PRESS PLAY", C_AMB);
        cv_text((240 - text_w(&FONT_T, "BY FLEXOR - BASED ON CHOPPAH")) / 2, 36, &FONT_T,   /* (0.268, FLEXOR: his
                                          * own firmware now, CHOPPAH credited) */
                "BY FLEXOR - BASED ON CHOPPAH", C_DIM);
        cv_blit(0, 190);
        ui.redraw = 0u;
    }
}

/* RESET ALL's screen: a brain seen from the side, an ice pick in through the eye socket, the frontal lobe gone grey,
 * a flatline under it, LOBOTOMIZED. Drawn whole, in four strips of 60 rows, from shapes (no picture in flash) */
#define LB_PINK RGB(255, 140, 160)
#define LB_FOLD RGB(190, 70, 100)
#define LB_EDGE RGB(120, 30, 60)
#define LB_DEAD RGB(96, 96, 104)
static int lobo_in(int32_t x, int32_t y)  /* inside the brain: the cerebrum (flat-bottomed), the cerebellum, the stem */
{
    float dx = (float)(x - 126) * (1.0f / 92.0f), dy = (float)(y - 88) * (1.0f / 60.0f);
    float cx = (float)(x - 176) * (1.0f / 36.0f), cy = (float)(y - 132) * (1.0f / 19.0f);
    if (dx * dx + dy * dy < 1.0f && dy < 0.66f)
        return 1;
    if (cx * cx + cy * cy < 1.0f)
        return 1;
    return x >= 146 && x < 162 && y >= 120 && y < 168;
}
static void draw_lobotomy(void)
{
    uint32_t band;
    for (band = 0; band < 4u; band++) {
        int32_t top = (int32_t)band * 60, y, x, i;
        cv_begin(240, 60, C_BLACK);
        cv_oy = -top;                    /* (draw in screen rows: the canvas shows top..top + 59) */
        for (y = top; y < top + 60 && y < 172; y++)
            for (x = 30; x < 222; x++) {
                float d, ex, ey;
                uint16_t c;
                if (!lobo_in(x, y))
                    continue;
                ex = (float)(x - 58);    /* the pick's tip, in the frontal lobe */
                ey = (float)(y - 96);
                if (!lobo_in(x - 1, y) || !lobo_in(x + 1, y) || !lobo_in(x, y - 1) || !lobo_in(x, y + 1))
                    c = LB_EDGE;
                else {                   /* the folds: two wavy fields crossing */
                    d = brk_sin_turns((float)x * 0.045f + 0.35f * brk_sin_turns((float)y * 0.03f + (float)x * 0.01f)) +
                        brk_sin_turns((float)y * 0.06f + 0.3f * brk_sin_turns((float)x * 0.025f));
                    c = brk_absf(d) < 0.14f ? LB_FOLD : LB_PINK;
                    if (x >= 140 && y >= 118)            /* the cerebellum: finer, level folds */
                        c = brk_sin_turns((float)y * 0.28f) > 0.6f ? LB_FOLD : LB_PINK;
                }
                if (ex * ex + ey * ey < 576.0f)          /* what the pick took: gone grey */
                    c = c == LB_PINK ? LB_DEAD : c == LB_FOLD ? RGB(52, 52, 58) : c;
                cv_rect(x, y, 1, 1, c);
            }
        for (i = 0; i <= 120; i++) {     /* the pick: a handle, then the shaft up to the tip */
            int32_t px = 4 + (58 - 4) * i / 120, py = 196 - (196 - 96) * i / 120;
            if (i < 40)
                cv_rect(px - 2, py - 2, 5, 5, C_AMB);
            else
                cv_rect(px, py, 2, 2, i > 112 ? C_WHITE : C_GRAY);
        }
        for (x = 96; x < 236; x++) {     /* the flatline, one last blip */
            int32_t b = x >= 112 && x < 124 ? (x < 116 ? -(x - 112) * 4 : x < 120 ? -16 + (x - 116) * 6 : 8 - (x - 120) * 2) : 0;
            cv_rect(x, 182 + b, 1, 2, x < 124 ? C_HI : C_GRAY);
        }
        cv_text2((240 - text2_w(&FONT_B, "LOBOTOMIZED")) / 2, 194, &FONT_B, "LOBOTOMIZED", C_HI);
        cv_text((240 - text_w(&FONT_XS, "ALL SETTINGS CLEARED")) / 2, 226, &FONT_XS, "ALL SETTINGS CLEARED", C_GRAY);
        cv_oy = 0;
        cv_blit(0, (uint32_t)top);
    }
}

/* the start screen's lights: a comet runs through the buttons as the break drops in (OCT- and OCT+, the top row left
 * to right, the bottom row back), a tail of mid-lit buttons behind it and the ones it passed glowing; then all flash
 * as the mark completes, and PLAY blinks green with PRESS PLAY. Returns 0 once the start screen is over */
/* the buttons' comet's way round: OCT- and OCT+, the top row left to right, the bottom row back (the demo's too) */
static const uint8_t PATH[NB] = {B_OCTDN, B_OCTUP, B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO,
                                 B_REC, B_PLAY, B_SEQ, B_ARP, B_SAVE, B_HOME};
static int intro_leds(uint32_t now)
{
    uint32_t t = now - ui.intro_t0, run = 16u * INTRO_STEP, b = 0u, mid = 0u, dim = 0u, i;
    if (!ui.intro)
        return 0;
    if (t < run) {                       /* the comet */
        uint32_t head = t * (NB + 2u) / run;   /* (+2: its tail runs out too) */
        for (i = 0; i < NB; i++)
            if (i == head)
                b |= 1u << PATH[i];
            else if (i + 2u >= head && i < head)
                mid |= 1u << PATH[i];
            else if (i < head)
                dim |= 1u << PATH[i];
    } else if (t < run + 160u)           /* the mark complete: all flash */
        b = ((1u << NB) - 1u) | LED_PLAY_GREEN;
    else {                               /* PRESS PLAY: PLAY blinks green, the rest glow */
        dim = ((1u << NB) - 1u) & ~(1u << B_PLAY);
        if ((t - run - 160u) % 500u < 250u)
            b = LED_PLAY_GREEN;
    }
    plat_lights(b, 0u, dim, 0u, 1);
    plat_lights_mid(mid, 0u);
    return 1;
}

static void intro_end(void)
{
    ui.intro = 0u;
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = RG_ALL;
}

/* the header's level meter: what leaves, left and right, -48..0 dBFS in 18 steps of 2.7 dB; amber from -8 dB, the
 * top white near full scale */
static void draw_meter(void)
{
    float lv[2];
    uint32_t c;
    int32_t h;
    lv[0] = meter_l;
    lv[1] = meter_r;
    cv_begin(8, 20, C_BLACK);
    for (c = 0; c < 2u; c++) {
        for (h = 0; h < 18; h++)         /* the steps it is above: -48 dB + 2.67 dB each (2^(dB / 6.02)) */
            if (lv[c] < brk_exp2f(((float)h * (48.0f / 18.0f) - 48.0f) * (1.0f / 6.0206f)))
                break;
        cv_rect((int32_t)c * 4, 1, 3, 18, C_BLOCK);
        if (h)
            cv_rect((int32_t)c * 4, 19 - h, 3, h, h >= 18 ? C_WHITE : h >= 15 ? C_AMB : C_HI);
    }
    cv_blit(ui.meter_x, 4);
}

/* the header: the break and the tempo, a play / stop mark; or a message for a moment */
/* the battery (CHOPPAH 1.0's, plat_battery): three bars from the foot, red at one or none, a USB host powering the
 * FM-1 shows a bolt instead. 9 wide, rows 4..20 (the header's canvas); -1 (not read yet): nothing */
#define BATT_W 9                         /* the body's width (the nub 3 wide on top of it) */
#define BATT_LOW RGB(255, 104, 72)       /* one bar or none: red */
static void draw_batt(int32_t x, int b)
{
    uint16_t c = b >= 0 && b <= 1 ? BATT_LOW : C_DIM;
    int32_t i;
    if (b < 0)
        return;
    cv_rect(x + 3, 4, 3, 2, c);           /* the nub */
    cv_rect(x, 6, BATT_W, 14, c);         /* the body's frame */
    cv_rect(x + 1, 7, BATT_W - 2, 12, C_BLACK);
    if (b == 4) {                        /* a USB host powers the FM-1: a bolt */
        cv_line(x + 5, 7, x + 2, 12, C_HI);
        cv_line(x + 2, 12, x + 6, 12, C_HI);
        cv_line(x + 6, 12, x + 3, 18, C_HI);
        return;
    }
    for (i = 0; i < b && i < 3; i++)      /* the bars from the foot up */
        cv_rect(x + 2, 15 - i * 4, BATT_W - 4, 3, b <= 1 ? BATT_LOW : C_HI);
}

static void draw_head(const brk_status_t *st, const brk_break_t *b, uint32_t now)
{
    char t[24];
    cv_begin(240, 28, C_BLACK);
    ui.meter_x = 0;
    if (now < ui.msg_until) {
        int32_t x = cv_text(8, 6, &FONT_B, ui.msg[0], C_WHITE);
        cv_text(x + 8, 6, &FONT_B, ui.msg[1], C_HI);
    } else if (tut_on()) {               /* LOAF BREAKER: the TUTORIAL: its step, what to do */
        char a[40], w[40];
        tut_head(a, w);
        cv_text(8, 2, &FONT_T, a, C_AMB);
        cv_text(8, 14, &FONT_T, w, tut_ok ? C_HI : C_WHITE);   /* (done: "OK!  NEXT IN 4 S") */
    } else {
        if (st->playing) {               /* a play triangle, or a stop square */
            int32_t r;
            for (r = 0; r < 6; r++)
                cv_rect(8 + r, 8 + r, 1, 13 - 2 * r, C_HI);
        } else
            cv_rect(8, 9, 10, 10, C_DIM);
        {                                /* the break, the pattern (> one queued), REC */
            int32_t x = cv_text(24, 6, &FONT_B, b ? hdr_brk_name() : "NO BREAK",   /* (amber: still to come) */
                                brk_next >= 0 ? C_AMB : C_WHITE);
            t[0] = 'P';
            int_text((int32_t)pat_cur() + 1, t + 1);
            if (pat_next >= 0)
                text_cat(t, ">");
            x = cv_text(x + 6, 10, &FONT_T, t, pat_next >= 0 ? C_AMB : C_GRAY);
            if (ui.trk_on) {             /* LOAF BREAKER: a track playing: "T1" */
                t[0] = 'T';
                int_text((int32_t)set.trk + 1, t + 1);
                x = cv_text(x + 5, 10, &FONT_T, t, C_HI);
            }
            if (ui.rec)
                cv_text(x + 5, 10, &FONT_T, "REC", C_AMB);
        }
        tenths_text(set.tempo_x10, t);
        cv_text(222 - text_w(&FONT_B, t), 6, &FONT_B, t,   /* (LOAF BREAKER: 10 left, the battery's room; TEST: white
                                          * on each beat) */
                st->playing && st->bar16 >= 0 && !(st->bar16 & 3) ? C_WHITE : C_HI);
        ui.meter_x = (int16_t)(218 - text_w(&FONT_B, t) - text_w(&FONT_T, "BPM") - 12);   /* left of BPM */
        cv_text(218 - text_w(&FONT_B, t) - text_w(&FONT_T, ext_active() ? "EXT" : "BPM"), 10, &FONT_T,
                ext_active() ? "EXT" : "BPM", ext_active() ? C_AMB : C_GRAY);
    }
    draw_batt(229, plat_battery());      /* LOAF BREAKER: the battery (CHOPPAH 1.0's), always at the right end */
    cv_rect(0, 27, 240, 1, now - ui.kick_t < 90u ? C_HI : C_LINE);   /* (TEST: lit on each kick) */
    if (st->playing && st->bar16 >= 0)   /* (TEST: the bar's beats, four ticks under the header, the one now lit) */
        cv_rect(2 + (st->bar16 >> 2) * 60, 26, 56, 1, C_GRAY);
    cv_blit(0, 0);
}

/* the whole break: its waveform, the slice markers, the slice sounding lit, the playhead (44 rows, under the tab
 * bar) */
static void draw_wave(const brk_status_t *st, const brk_break_t *b)
{
    uint32_t i;
    cv_begin(240, 44, C_BLACK);
    if (b && b->len) {
        int32_t lit_a = -1, lit_z = -1;
        if (st->slice >= 0) {
            lit_a = (int32_t)(b->slice[st->slice] * 240u / b->len);
            lit_z = (int32_t)(b->slice[st->slice + 1] * 240u / b->len);
            cv_rect(lit_a, 1, lit_z - lit_a, 42, C_BLOCK);
            if (set.gate && set.gate < 100u && st->key < 0)   /* gated: only the part that sounds lit */
                lit_z = lit_a + ((lit_z - lit_a) * (int32_t)set.gate + 99) / 100;
        }
        if (strip_style() == SS_GREY)    /* (GREY: as it always was, the peaks) */
            for (i = 0; i < 240u; i++) {
                int32_t h = wave_px[i] * 5 / 6;
                cv_rect((int32_t)i, 22 - h, 1, 2 * h + 1, (int32_t)i >= lit_a && (int32_t)i < lit_z ? C_HI : C_DIM);
            }
        else {                           /* (0.9.70, FLEXOR: in colour, by its level as heard; the beats behind, the
                                          * bar's first brighter; out of the slice sounding a little darker) */
            uint32_t nb = (b->bars ? b->bars : 2u) * 4u, k;
            for (k = 1u; k < nb; k++)
                cv_rect((int32_t)(k * 240u / nb), 6, 1, 38, k % 4u ? C_LINE : C_DIM);
            for (i = 0; i < 240u; i++)
                wave_col((int32_t)i, 22, wave_b[i][0], wave_b[i][1], wave_b[i][2], wave_b[i][3],
                         (int32_t)i >= lit_a && (int32_t)i < lit_z ? 255u : 185u, SS_WS[strip_style()], 0);
        }
        for (i = 1; i < b->nslices; i++)
            cv_rect((int32_t)(b->slice[i] * 240u / b->len), 0, 1, 5, C_GRAY);
        if (st->slice >= 0)
            cv_rect((int32_t)((uint32_t)st->pos * 240u / b->len), 0, 1, 44, C_WHITE);
    }
    cv_blit(0, 44);
}

static void draw_blocks(const brk_status_t *st, uint32_t keys)
{
    uint32_t row, k, c;
    char t[4];
    for (row = 0; row < 2u; row++) {
        cv_begin(240, 52, C_BLACK);
        for (k = row * 8u; k < row * 8u + 8u; k++) {
            int32_t x = 1 + (int32_t)(k % 8u) * 30, held = (keys >> WK[k]) & 1u;
            int on = ui.rec && !st->playing ? k == rec_step % 16u   /* LOAF BREAKER: REC stopped, the step the
                                          * next key writes (its half: REC STEP's popup) */
                                            : st->slice == (int32_t)k, have = k < blk_n;
            uint16_t ground = on ? C_BLOCK_HI : C_BLOCK, ink = on ? C_BLACK : have ? C_GRAY : C_LINE;
            cv_rect(x, 0, 28, 50, held ? C_WHITE : ground);
            if (held)
                cv_rect(x + 2, 2, 24, 46, ground);
            if (have)
                for (c = 0; c < BLK_W - 4u; c++) {   /* the slice's shape, 22 columns of its 26 */
                    int32_t h = blk_px[k][c * BLK_W / (BLK_W - 4u)];
                    cv_rect(x + 3 + (int32_t)c, 19 - h / 2, 1, h + 1, ink);
                }
            int_text((int32_t)k + 1, t);
            cv_text(x + 3, 35, &FONT_T, t, on ? C_BLACK : have ? C_AMB : C_LINE);
            if (have && key_custom((int)k))  /* a loop of its own: a dot */
                cv_rect(x + 21, 39, 4, 4, on ? C_BLACK : C_HI);
            if (have && (set.slx[k].type || set.slx[k].level < 100u))   /* an effect of its own: its tag */
                cv_text(x + 13, 35, &FONT_XS, set.slx[k].type ? SLX_TAG[set.slx[k].type] : "LV",
                        on ? C_BLACK : C_HI);
        }
        cv_blit(0, 88u + row * 52u);
    }
}
