/* SPDX-License-Identifier: MPL-2.0 */
/* K-GLITCH 0.2b: the TUTORIAL taken out for room (LOAF BREAKER's tutor.c: 30 guided steps). What is left: a new
 * FM-1's question, BEGINNER or EXPERT (set.tut TUT_ASK), and what the other files call of the tutorial, inert. */
#define TUT_DONE 0xFEu
#define TUT_NAV 0u
enum { TC_EDIT = 7 };
static const struct { uint8_t cond; } TUT[1] = {{0u}};
static const uint32_t tut_ok = 0u;
static int tut_on(void) { return 0; }
static void tut_start(void) {}
static uint32_t tut_btn_ok(void) { return 0u; }
static void tut_frame(const brk_status_t *st) { (void)st; }
static int tut_home_stay(void) { return 0; }
static void tut_skip(void) {}
static void tut_head(char *a, char *b) { a[0] = b[0] = 0; }
static void tut_leds(uint32_t *b, uint32_t *k, int playing) { (void)b; (void)k; (void)playing; }
#define TUT_ASK 0xFDu
static uint8_t ask_sel = 1u, ask_drawn = 0xFFu;
static const char *const ASK_NAME[3] = {"", "BEGINNER", "EXPERT"};
static const char *const ASK_TEXT[3][2] = {{"26 GUIDED STEPS", "THE BUTTONS TO PRESS BLINK"},
                                           {"THE ESSENTIALS", "PLAY RIGHT AWAY"},
                                           {"EVERYTHING", "EVERY SETTING, EVERY PAGE"}};
static void tut_quit(void);
static void ask_pick(uint32_t c)
{
    set.novice = c < 2u ? 1u : 0u;
    set.tut = TUT_DONE;
    if (set.novice)
        novice_lengths();
    if (c == 1u)                         /* (0.250, FLEXOR: a first start, BEGINNER: AUTO SAVE on; the TUTORIAL: off
                                          * while it runs, on after it; EXPERT: as it was) */
        set.auto_save = 1u;
    app_say("MODE", ASK_NAME[c % 3u]);
    set_dirty = 1u;
    changed();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = RG_ALL;
}
static void ask_draw(void)
{
    uint32_t c;
    if (ask_drawn == ask_sel)
        return;
    if (ask_drawn == 0xFFu)
        lcd_fill(0, 0, 240, 240, C_BLACK);
    ask_drawn = ask_sel;
    cv_begin(240, 60, C_BLACK);          /* the question */
    cv_text2((240 - text2_w(&FONT_B, "YOUR MODE")) / 2, 1, &FONT_B, "YOUR MODE", C_WHITE);
    cv_text((240 - text_w(&FONT_XS, "TURN A KNOB, PRESS PLAY")) / 2, 33, &FONT_XS, "TURN A KNOB, PRESS PLAY", C_AMB);
    cv_text((240 - text_w(&FONT_XS, "LATER: GLO, SETUP, KNOB 3")) / 2, 44, &FONT_XS, "LATER: GLO, SETUP, KNOB 3", C_DIM);
    cv_blit(0, 0);
    for (c = 1; c < 3u; c++) {           /* (K-GLITCH 0.2b: two, no TUTORIAL) */
        int on = c == ask_sel;
        char n[16];
        cv_begin(240, 60, C_BLACK);
        cv_rect(12, 3, 216, 50, on ? C_HI : C_BLOCK);
        n[0] = (char)('0' + c);
        n[1] = 0;
        cv_text(22, 9, &FONT_B, n, on ? C_BLACK : C_DIM);
        cv_text(42, 9, &FONT_B, ASK_NAME[c], on ? C_BLACK : C_WHITE);
        cv_text(42, 28, &FONT_XS, ASK_TEXT[c][0], on ? C_BLACK : C_GRAY);
        cv_text(42, 39, &FONT_XS, ASK_TEXT[c][1], on ? C_BLACK : C_GRAY);
        cv_blit(0, 60u + (c - 1u) * 60u);
    }
}
static void ask_input(uint32_t kdn, uint32_t bdn, int32_t d)
{
    uint32_t i;
    if (d)
        ask_sel = (uint8_t)clamp((int32_t)ask_sel + (d > 0 ? 1 : -1), 1, 2);
    for (i = 0; i < 2u; i++)
        if (kdn >> WK[i] & 1u) {
            ask_pick(i + 1u);
            return;
        }
    if (bdn & 1u << B_PLAY)
        ask_pick(ask_sel);
}
static void tut_quit(void)
{
    set.tut = TUT_DONE;
    set.auto_save = 1u;                  /* (0.250: on after the tutorial) */
    set_dirty = 1u;
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
