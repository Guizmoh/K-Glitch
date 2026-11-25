/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal (felucca-bsp loader/ldr_screen.c); CHOPPAH's look, 2026; LOAF BREAKER's, 2026 */
/* LOAF BREAKER's update screen: the update loader's own (felucca-bsp loader/ldr_screen.c, which build.sh swaps this in
 * for while the firmware builds, ldr_loaf.h beside it). Black: LOAF BREAKER (its face and lettering, ldr_loaf.h,
 * tools/make_egg.py) at the top, "UPDATING" in white and "DO NOT UNPLUG" in red under it, and at the very bottom the
 * slice mark (sixteen bars, two chopped out of line) filling in white as the app area is written: the progress (the
 * segmented bar and CHOPPAH's name are gone). Only the drawing differs from the original: the panel's start (reset,
 * sleep out, RGB565, IPS inversion), ldr_screen_init and ldr_screen_progress are the same, and every buffer is in RAM
 * (the SPI DMA reads RAM only; the picture is decoded into scr_strip). LOAF BREAKER on fm1-base's 1.0 loader (as
 * CHOPPAH 1.0's): before the install a lit bar runs through the mark (waiting); a host silent 2 s mid-install turns
 * the lines to "USB LOST" / "RECONNECT THE CABLE" (its next frame turns them back); the end, "DONE" / "RESTARTING".
 * Part of loader.c's single unit. */
#include "../boards/fm1/fm1_lcd_hw.h"
#include "ldr_loaf.h"

#define SCR_BG 0x0000u                              /* black */
#define SCR_FG 0xFFFFu                              /* white: UPDATING, the mark written */
#define SCR_AMB 0xF8C3u                             /* #ff1818 red: DO NOT UNPLUG, the two chopped bars */
#define SCR_DIM 0x2945u                             /* #282828: the mark, not yet written */
#define SCR_LOAF_Y 2u                               /* the picture's top */
#define SCR_MARK_B 236u                             /* the mark's bars' foot (the chopped two 8 px higher) */
#define SCR_ROWS 21u                                /* the tallest line: 5x7 glyphs at 3x */
#define SCR_LINE1 (SCR_LOAF_Y + LDR_LOAF_H + 3u)    /* UPDATING's row */
#define SCR_LINE2 (SCR_LOAF_Y + LDR_LOAF_H + 28u)   /* DO NOT UNPLUG's */
#define SCR_LOST_POLLS 4000u                        /* 2 s of loader.c's 0.5 ms USB polls without a frame */
#define SCR_SWEEP_POLLS 200u                        /* the waiting bar a step every 100 ms of polls */

static uint16_t scr_strip[240u * SCR_ROWS];         /* one line of text, byte-swapped (the panel takes MSB first) */
static uint8_t scr_small[8];
static uint32_t scr_marks;                          /* the mark's bars lit */
static uint8_t scr_on, scr_armed;                   /* armed: an install has started */
static uint8_t scr_state;                           /* 0 updating, 1 USB lost, 2 done */
static uint8_t scr_sweep;                           /* the waiting bar lit + 1 (0: none) */
static uint32_t scr_quiet, scr_wait;                /* polls since the host's last frame; polls waiting */

/* 5x7 glyphs, a row a byte (bit 4 = left) */
static const struct { char c; uint8_t rows[7]; } SCR_FONT[] = {
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
    { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
    { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
};

/* the mark: sixteen bars like a break's hits (as the app's start screen draws it) */
static const uint8_t SCR_MARK_H[16] = {30, 12, 26, 10, 34, 14, 11, 20, 12, 22, 28, 24, 34, 10, 13, 22};

static void scr_cmd(uint8_t c)
{
    fm1_lcd_send_cmd(c);
    fm1_lcd_wait();
    fm1_lcd_deselect();
}

static void scr_data(const void *p, uint32_t n)     /* n > 0 */
{
    fm1_lcd_send_data(p, n);
    fm1_lcd_wait();
    fm1_lcd_deselect();
}

static void scr_window(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
    scr_small[0] = (uint8_t)(x0 >> 8); scr_small[1] = (uint8_t)x0;
    scr_small[2] = (uint8_t)(x1 >> 8); scr_small[3] = (uint8_t)x1;
    scr_cmd(0x2A);
    scr_data(scr_small, 4);
    scr_small[4] = (uint8_t)(y0 >> 8); scr_small[5] = (uint8_t)y0;
    scr_small[6] = (uint8_t)(y1 >> 8); scr_small[7] = (uint8_t)y1;
    scr_cmd(0x2B);
    scr_data(scr_small + 4, 4);
    scr_cmd(0x2C);
}

static uint16_t scr_swap(uint16_t c) { return (uint16_t)((c >> 8) | (c << 8)); }

static void scr_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, n = w * h, k = 240u * SCR_ROWS;
    if (!n)
        return;
    for (i = 0; i < k && i < n; i++)
        scr_strip[i] = scr_swap(c);
    scr_window(x, y, x + w - 1u, y + h - 1u);
    for (; n; n -= k) {
        if (k > n)
            k = n;
        scr_data(scr_strip, k * 2u);
    }
}

static void scr_text(uint32_t y, const char *s, uint32_t scale, uint16_t fg)   /* one line, centred; scale <= 3 */
{
    uint32_t len = 0, i, r, b, sx, sy, x0, w;
    while (s[len])
        len++;
    w = len * 6u * scale - scale;
    x0 = (240u - w) / 2u;
    for (i = 0; i < w * 7u * scale; i++)
        scr_strip[i] = scr_swap(SCR_BG);
    for (i = 0; i < len; i++) {
        const uint8_t *g = 0;
        uint32_t f;
        for (f = 0; f < sizeof SCR_FONT / sizeof SCR_FONT[0]; f++)
            if (SCR_FONT[f].c == s[i])
                g = SCR_FONT[f].rows;
        if (!g)
            continue;                               /* a space */
        for (r = 0; r < 7u; r++)
            for (b = 0; b < 5u; b++)
                if (g[r] & (0x10u >> b))
                    for (sy = 0; sy < scale; sy++)
                        for (sx = 0; sx < scale; sx++)
                            scr_strip[(r * scale + sy) * w + (i * 6u + b) * scale + sx] = scr_swap(fg);
    }
    scr_window(x0, y, x0 + w - 1u, y + 7u * scale - 1u);
    scr_data(scr_strip, w * 7u * scale * 2u);
}

static void scr_mark(uint32_t k, int lit)          /* bar k of the mark, written (lit) or still to come */
{
    uint32_t h = SCR_MARK_H[k], chop = k == 5u || k == 11u, y = SCR_MARK_B - (chop ? 8u : 0u) - h;
    scr_fill(8u + k * 14u, y, 12u, h, lit ? (chop ? SCR_AMB : SCR_FG) : SCR_DIM);
}

/* LOAF BREAKER, centred at the top: ldr_loaf.h's runs (a byte a run: bits 0-3 the grey, 4-7 the length less one),
 * decoded into scr_strip as many rows at a time as it holds, each batch sent as one window */
static void scr_loaf(void)
{
    uint32_t per = 240u * SCR_ROWS / LDR_LOAF_W, i = 0, x = 0, y = 0, n = 0;
    uint32_t x0 = (240u - LDR_LOAF_W) / 2u;
    while (y < LDR_LOAF_H) {
        uint32_t rows = LDR_LOAF_H - y < per ? LDR_LOAF_H - y : per, row = 0;
        n = 0;
        while (row < rows && i < sizeof LDR_LOAF_RLE) {
            uint32_t b = LDR_LOAF_RLE[i++], k = (b >> 4) + 1u;
            uint16_t c = scr_swap(LDR_LOAF_PAL[b & 15u]);
            while (k--) {
                scr_strip[n++] = c;
                if (++x == LDR_LOAF_W) {
                    x = 0;
                    row++;
                }
            }
        }
        scr_window(x0, SCR_LOAF_Y + y, x0 + LDR_LOAF_W - 1u, SCR_LOAF_Y + y + rows - 1u);
        scr_data(scr_strip, n * 2u);
        y += rows;
    }
}

static void ldr_screen_init(void)
{
    uint32_t k;
    fm1_lcd_hw_init();                              /* pins, backlight on, SPI1 */
    fm1_delay_ms(20);
    scr_cmd(0x01);                                  /* SWRESET */
    fm1_delay_ms(150);
    scr_cmd(0x11);                                  /* SLPOUT */
    fm1_delay_ms(120);
    scr_cmd(0x3A);                                  /* COLMOD: RGB565 */
    scr_small[0] = 0x55;
    scr_data(scr_small, 1);
    scr_cmd(0x36);                                  /* MADCTL: top-left origin, RGB order */
    scr_small[0] = 0x00;
    scr_data(scr_small, 1);
    scr_cmd(0x21);                                  /* INVON: IPS panel */
    scr_cmd(0x13);                                  /* NORON */
    scr_fill(0, 0, 240, 240, SCR_BG);
    scr_loaf();
    scr_text(SCR_LINE1, "UPDATING", 3, SCR_FG);
    scr_text(SCR_LINE2, "DO NOT UNPLUG", 2, SCR_AMB);
    for (k = 0; k < 16u; k++)
        scr_mark(k, 0);
    scr_cmd(0x29);                                  /* DISPON */
    scr_on = 1;
}

static void ldr_screen_progress(uint32_t done, uint32_t total)
{
    uint32_t marks;
    if (!scr_on || !total)
        return;
    if (!scr_armed && scr_sweep)                    /* the install starts: the waiting bar goes */
        scr_mark(scr_sweep - 1u, 0);
    scr_armed = 1;
    marks = done >= total ? 16u : done * 16u / total;   /* < 2^32: total <= 2 MiB */
    while (scr_marks < marks)                       /* the mark fills in, a bar a sixteenth: the progress */
        scr_mark(scr_marks++, 1);
}

/* the two lines again, each across the whole width first (a shorter one leaves nothing of the last) */
static void scr_lines(const char *a, const char *b)
{
    scr_fill(0, SCR_LINE1, 240, 21u, SCR_BG);
    scr_text(SCR_LINE1, a, 3, SCR_FG);
    scr_fill(0, SCR_LINE2, 240, 14u, SCR_BG);
    scr_text(SCR_LINE2, b, 2, SCR_AMB);
}

/* waiting, before the install: one bar of the mark lit, a step on */
static void scr_sweep_step(void)
{
    if (scr_sweep)
        scr_mark(scr_sweep - 1u, 0);
    scr_sweep = (uint8_t)(scr_sweep % 16u + 1u);
    scr_mark(scr_sweep - 1u, 1);
}

/* a frame from the host (cheap: called for each one): it is there, the lines go back to updating */
static void ldr_screen_heard(void)
{
    scr_quiet = 0;
    if (scr_state == 1u) {
        scr_state = 0;
        scr_lines("UPDATING", "DO NOT UNPLUG");
    }
}

/* one USB poll (0.5 ms) without a frame: before the install the waiting bar steps; an install whose host stays silent
 * 2 s is shown lost, once */
static void ldr_screen_tick(void)
{
    if (!scr_on)
        return;
    if (!scr_armed) {
        if (++scr_wait >= SCR_SWEEP_POLLS) {
            scr_wait = 0;
            scr_sweep_step();
        }
        return;
    }
    if (scr_state || ++scr_quiet < SCR_LOST_POLLS)
        return;
    scr_state = 1;
    scr_lines("USB LOST", "RECONNECT THE CABLE");
}

/* written and verified: the app starts next */
static void ldr_screen_done(void)
{
    if (!scr_on)
        return;
    ldr_screen_progress(1, 1);
    scr_state = 2;
    scr_lines("DONE", "RESTARTING");
}
