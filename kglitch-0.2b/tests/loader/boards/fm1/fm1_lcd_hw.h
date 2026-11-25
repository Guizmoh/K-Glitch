/* SPDX-License-Identifier: MPL-2.0 */
/* LOAF BREAKER: a stand-in for felucca-bsp's FM-1 panel (boards/fm1/fm1_lcd_hw.h) on a computer, for
 * tests/loader/src/preview.c: the ST7789's column / row windows and RAM writes, into a 240 x 240 RGB565 picture */
#include <stdint.h>

static uint16_t lcd_fb[240 * 240];
static uint8_t lcd_cmd, lcd_args[4], lcd_nargs, lcd_hi = 1, lcd_hb;
static uint32_t lcd_x0, lcd_x1, lcd_y0, lcd_y1, lcd_x, lcd_y;

static void fm1_lcd_hw_init(void) {}
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
static void fm1_lcd_wait(void) {}
static void fm1_lcd_deselect(void) {}
static void fm1_lcd_send_cmd(uint8_t c)
{
    lcd_cmd = c;
    lcd_nargs = 0;
    if (c == 0x2C) {                     /* RAMWR: from the window's corner */
        lcd_x = lcd_x0;
        lcd_y = lcd_y0;
        lcd_hi = 1;
    }
}
static void fm1_lcd_send_data(const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (lcd_cmd == 0x2A || lcd_cmd == 0x2B) {   /* CASET / RASET: start and end, MSB first */
            if (lcd_nargs < 4)
                lcd_args[lcd_nargs++] = b[i];
            if (lcd_nargs == 4) {
                uint32_t a = (uint32_t)lcd_args[0] << 8 | lcd_args[1], z = (uint32_t)lcd_args[2] << 8 | lcd_args[3];
                if (lcd_cmd == 0x2A) {
                    lcd_x0 = a;
                    lcd_x1 = z;
                } else {
                    lcd_y0 = a;
                    lcd_y1 = z;
                }
            }
        } else if (lcd_cmd == 0x2C) {    /* a pixel: two bytes, MSB first */
            if (lcd_hi) {
                lcd_hb = b[i];
                lcd_hi = 0;
            } else {
                if (lcd_x < 240u && lcd_y < 240u)
                    lcd_fb[lcd_y * 240u + lcd_x] = (uint16_t)(lcd_hb << 8 | b[i]);
                lcd_hi = 1;
                if (++lcd_x > lcd_x1) {
                    lcd_x = lcd_x0;
                    lcd_y++;
                }
            }
        }
    }
}
