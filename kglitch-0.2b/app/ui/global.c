/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the GLOBAL page: MIDI clock, Bluetooth MIDI, pitch, snap, RESET ALL. (Part of app.c's unity: included
 * there in order.) */

static void reset_all(void);             /* (below, with the break's change) */
static void bk_release_all(void);        /* (ovl.c: whatever a black key had on lets go) */
/* the GLOBAL page's knobs: clock in, clock out, Bluetooth, where MIDI goes */
static uint8_t *sfx_knob(int i)          /* LOAF BREAKER: GLO EFFECTS' K1-K4: AMOUNT, on a KICK, at the DROP, LENGTH */
{
    return i == 0 ? &set.scrfx : i == 1 ? &set.sfx_kick : i == 2 ? &set.sfx_drop : &set.sfx_len;
}
static void sfx_toggle(uint32_t e, int on)   /* LOAF BREAKER: GLO EFFECTS: effect e (SFX_*) on (1), off (0), or turned
                                          * over (-1) */
{
    uint8_t b = (uint8_t)(1u << (e - 1u));
    set.sfx_on = (uint8_t)(on < 0 ? set.sfx_on ^ b : on ? set.sfx_on | b : set.sfx_on & ~b);
    ui.sfx_cur = (uint8_t)e;
    changed();
    app_say(SFX_NAME[e], set.sfx_on & b ? "ON" : "OFF");
    ui.redraw = RG_ALL;
}
/* LOAF BREAKER (0.201, the pages by domain): two parts of this page's code are shown elsewhere: GP_BREAK, the break's
 * PITCH, GRAIN, SNAP and LEVEL, is SOUNDS' BREAK (SEL + EDIT); GP_FILL, SWING and FILL, is PATTERN's GROOVE (SEQ's
 * ALGORITHM past KEYS, SEL + ARP). SYSTEM (GLO) itself: MIDI, SETUP, EFFECTS, FX SET, RESET */
#define GP_BREAK 1u
#define GP_FILL 6u
#define GP_LIGHTS 7u                     /* (0.9.57, FLEXOR: the LEDs: GLOW and the PULSE) */
#define NGLO 7u
#define GLO_HSET 0xF0u                   /* (0.9.56, FLEXOR: HOME's SETTINGS, its page PG_HSET, now SYSTEM's, after SETUP) */
static const uint8_t GLO_ORD[NGLO] = {0u, 2u, GLO_HSET, 3u, 4u, GP_LIGHTS, 5u};
static uint32_t glo_next(int32_t d)      /* SYSTEM's next or previous part (from the break's or GROOVE: MIDI) */
{
    uint32_t i, cur = ui.page == PG_HSET ? GLO_HSET : ui.gpage;
    for (i = 0; i < NGLO; i++)
        if (GLO_ORD[i] == cur && (ui.page == PG_GLOBAL || ui.page == PG_HSET))
            return GLO_ORD[(i + (d > 0 ? 1u : NGLO - 1u)) % NGLO];
    return 0u;
}
static int tut_on(void);                 /* (LOAF BREAKER: the TUTORIAL, tutor.c: further on) */
static void tut_start(void);
static void tut_quit(void);
/* LOAF BREAKER (0.9.57, FLEXOR): SYSTEM › LIGHTS: K1 GLOW (the lights at rest), K2 PULSE (on the tempo while PLAY
 * runs: OFF, each KICK, 1/8, 1/4, 1/2, a BAR), K3 PULSE ON (which lights), K4 STYLE; set.leds, app.c led_cfg */
static const char *const LP_NAME[LP_N] = {"OFF", "KICK", "1/8", "1/4", "1/2", "BAR"};
static const char *const LO_NAME[LO_N] = {"BUTTONS", "WHITE KEYS", "BLACK KEYS", "ALL KEYS", "EVERYTHING"};
static const char *const LS_NAME[LS_N] = {"FLASH", "FADE", "BREATHE", "BLINK", "CHASE"};
static uint32_t lt_lb, lt_lk, lt_mb, lt_mk, lt_db, lt_dk;   /* (the lights as main.c's leds() last set them: lit,
                                          * mid, glowing; buttons B_*, keys; the page's picture) */
static void lights_value(int i, char *t)
{
    uint32_t v = led_cfg((uint32_t)i);
    t[0] = 0;
    if (i == 0) {
        if (v)
            int_text((int32_t)v, t);
        else
            text_cat(t, "OFF");
    } else
        text_cat(t, i == 1 ? LP_NAME[v % LP_N] : i == 2 ? LO_NAME[v % LO_N] : LS_NAME[v % LS_N]);
}
static void lights_turn(int i, int32_t d)
{
    static const char *const LN[4] = {"GLOW", "PULSE", "PULSE ON", "PULSE STYLE"};
    static const uint8_t N[4] = {LED_GLOWS, LP_N, LO_N, LS_N};
    char t[16];
    i &= 3;
    led_put((uint32_t)i, (uint32_t)clamp((int32_t)led_cfg((uint32_t)i) + (d > 0 ? 1 : -1), 0, (int32_t)N[i] - 1));
    set_dirty = 1u;
    lights_value(i, t);
    app_say(LN[i], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void global_value(int i, char *t)
{
    t[0] = 0;
    if (ui.gpage == GP_FILL) {           /* (SEQ's SETTINGS: SWING; 0.9.31, FLEXOR: BREAK ROWS, FOLLOW) */
        if (i == 0) {
            int_text(set.swing, t);
            text_cat(t, "%");
        } else if (i == 1)
            text_cat(t, bk_seq() & 2u ? "16 STEPS" : "32 STEPS");
        else if (i == 2)
            text_cat(t, bk_seq() & 1u ? "OFF" : "ON");
        return;
    }
    if (ui.gpage == GP_LIGHTS) {         /* (0.9.57: LIGHTS) */
        lights_value(i, t);
        return;
    }
    if (ui.gpage == 4u) {                /* (FX SET: each 0..100 %) */
        pct_text((float)*sfx_knob(i) * 0.01f, t);
        return;
    }
    if (ui.gpage == 3u) {                /* (EFFECTS: K1 the one pointed at, K2 it on or off) */
        uint32_t e = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
        if (i == 0)
            text_cat(t, SFX_NAME[e]);
        else if (i == 1)
            text_cat(t, SFX_ON(e) ? "ON" : "OFF");
        else if (i == 2)
            text_cat(t, DFT_NAME[dft_kind()]);   /* (0.9.67, FLEXOR: DRUM FILTER, its kind) */
        else if (i == 3)
            text_cat(t, KSW_NAME[ksw_mode()]);   /* (0.9.64, FLEXOR: KICK SWITCH) */
        return;                          /* (K4 EDIT VIEW and PRESETS DRUM VIEW ZOOM until 0.9.28: KICK › KNOBS › TOOLS) */
    }
    if (ui.gpage == 2u) {                /* SETUP (FLEXOR, 0.213: the knobs in the screen's order, top to bottom): K1 SKIN,
                                          * K2 WAVE ZOOM, K3 MODE, K4 AUTO SAVE */
        if (i == 0)                      /* (LOAF BREAKER, 0.237: the names nine letters at most, to fit its column) */
            text_cat(t, SKIN_NAME[skin_idx()]);
        else if (i == 1)                 /* (0.9.57, FLEXOR: KICK EQ, EXPERT's; WAVE ZOOM until 0.9.1) */
            text_cat(t, keq_flat ? "OFF (FLAT)" : "ON");
        else if (i == 2)                 /* (TUTORIAL, left of BEGINNER) */
            text_cat(t, tut_on() ? "TUTORIAL" : set.novice ? "BEGINNER" : "EXPERT");
        else
            text_cat(t, set.auto_save ? "ON" : "OFF");
        return;
    }
    if (ui.gpage) {                      /* the pitch: speed or keep length, the grains */
        if (i == 0)
            text_cat(t, pitch_keeps() ? "KEEP LEN" : "SPEED");
        else if (i == 1) {
            int_text(set.grain_ms ? set.grain_ms : 30, t);
            text_cat(t, " MS");
        } else if (i == 2)
            text_cat(t, SNAP_SHORT[snap_index()]);
        else {                           /* (LOAF BREAKER: K4 the break's LEVEL, the MIXER's BREAK; RESET ALL: SYSTEM
                                          * RESET, EVERYTHING) */
            int_text(set.brk_level, t);
            text_cat(t, "%");
        }
        return;
    }
    text_cat(t, i == 0 ? (set.clk_in ? "FOLLOW" : "OFF") : i == 1 ? (set.clk_out ? "ON" : "OFF")
                : i == 2 && !set.novice ? (ui.meter_on ? "ON" : "OFF")
                : "");                   /* (LOAF BREAKER: K3 the METER, EXPERT only; K4 BREAK TIME 0.256-0.9.1: HOME's
                                          * SETTINGS now) */
}

/* (0.9.64, FLEXOR: a TR-8S's notes cutting the break) SYSTEM › MIDI's PRESETS: NOTES IN, ALL (the slices 48-63 and
 * the drums 36-46, as before), DRUMS (the drums alone), OFF (no notes: the clock, start, stop alone) */
static void notes_in_turn(int32_t d)
{
    notes_in_put((uint32_t)clamp((int32_t)notes_in() + (d > 0 ? 1 : -1), 0, NI_N - 1));
    set_dirty = 1u;
    app_say("NOTES IN", notes_in() == NI_ALL ? "ALL: SLICES AND DRUMS" : notes_in() == NI_DRUMS
            ? "DRUMS ONLY: 36-46" : "OFF: CLOCK ALONE");
    ui.redraw |= RG_BLOCKS;
}
/* LOAF BREAKER (FLEXOR, 11 Nov 2026): BEGINNER's BREAK its LEVEL alone (K4; the pitch: OCT), GROOVE its SWING alone */
static int glo_knob_ok(int i)
{
    return !set.novice || ui.page != PG_GLOBAL || (ui.gpage == GP_FILL ? i < 3 : ui.gpage == GP_BREAK ? i == 3 : ui.gpage == 2u ? i != 1 : 1);
}
static void global_turn(int i, int32_t d)
{
    static const char *const NAME[4] = {"CLOCK IN", "CLOCK OUT", "BLUETOOTH", "BREAK TIME"};
    char t[16];
    if (!glo_knob_ok(i)) {
        easy_no();
        return;
    }
    if (ui.gpage == GP_FILL) {           /* LOAF BREAKER: SEQ's SETTINGS (GROOVE until 0.9.30): K1 SWING; 0.9.31, FLEXOR:
                                          * K2 BREAK ROWS, the FULL view's (right 32), K3 FOLLOW, the grid's bar the one
                                          * playing (right ON) */
        static const char *const SN[3] = {"SWING", "BREAK ROWS", "FOLLOW"};
        if (i == 0)
            set.swing = (uint8_t)clamp((int32_t)set.swing + d, 50, 75);
        else if (i < 3) {
            uint32_t b = i == 1 ? 2u : 1u;
            set.bkeys.bk[BK_SEQ] = (uint8_t)(0xD0u | (d > 0 ? bk_seq() & ~b : bk_seq() | b));
            if (brk_view())              /* (the FULL view's rows now) */
                set.brkv = (uint8_t)(BRKV_TAG | (bk_seq() & 2u ? 1u : 2u));
        } else
            return;
        set_dirty = 1u;
        global_value(i, t);
        app_say(SN[i], t);
        ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
        return;
    }
    if (ui.gpage == 3u) {                /* LOAF BREAKER: EFFECTS: K1 the effect, K2 it on (right) or off (left) */
        uint32_t e = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
        if (i == 0) {
            ui.sfx_cur = (uint8_t)clamp((int32_t)e + (d > 0 ? 1 : -1), SFX_CRT, SFX_N - 1);
            app_say("EFFECT", SFX_NAME[ui.sfx_cur]);
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else if (i == 1)
            sfx_toggle(e, d > 0);
        else if (i == 2) {               /* (0.9.67, FLEXOR: K3 DRUM FILTER, the drums' filter's kind: CLASSIC,
                                          * SMOOTH (a club mixer's), SOFT (6 dB), RESO) */
            set.click_gone = (uint8_t)(0x60u | (uint32_t)clamp((int32_t)dft_kind() + (d > 0 ? 1 : -1), 0, BRK_DFTS - 1));
            set_dirty = 1u;
            app_say("DRUM FILTER", dft_kind() == BRK_DFT_SOFT ? "SOFT (6 DB)" : DFT_NAME[dft_kind()]);
            ui.redraw |= RG_FOOT;
        } else if (i == 3) {             /* (0.9.64, FLEXOR: K4 KICK SWITCH, the drums' filter as KIL brings the kicks
                                          * back: SMART, FLT OFF, FLT KEEP) */
            ksw_put((uint32_t)clamp((int32_t)ksw_mode() + (d > 0 ? 1 : -1), 0, KSW_N - 1));
            set_dirty = 1u;
            app_say("KICK SWITCH", ksw_mode() == KSW_SMART ? "SMART: FILTER SET DURING KIL STAYS"
                    : ksw_mode() == KSW_OFF ? "FILTER OFF AT KICKS BACK" : "FILTER ALWAYS KEPT");
            ui.redraw |= RG_FOOT;
        }
                                         /* (K3 WAVE until 0.9.24: HOME's SETTINGS K2 alone; K4 EDIT VIEW until 0.9.28:
                                          * KICK › MORE › TOOLS, FLEXOR; neither named here since 0.9.30) */
        return;
    }
    if (ui.gpage == GP_LIGHTS) {         /* (0.9.57: LIGHTS) */
        lights_turn(i, d);
        return;
    }
    if (ui.gpage == 4u) {                /* LOAF BREAKER: FX SET: K1 AMOUNT, K2 KICK, K3 DROP, K4 LENGTH, 5 % a detent */
        static const char *const SN[4] = {"EFFECT AMOUNT", "ON A KICK", "AT THE DROP", "MOMENTS' LENGTH"};
        uint8_t *p = sfx_knob(i);
        *p = (uint8_t)clamp((int32_t)*p + d * 5, 0, 100);
        changed();
        global_value(i, t);
        app_say(SN[i & 3], t);
        ui.redraw = RG_ALL;
        return;
    }
    if (ui.gpage == 2u) {                /* SETUP: K1 the skin, the whole screen in its colours at once */
        if (i == 0) {
            set.skin = (uint8_t)(SKIN_NEW | (skin_idx() + (d > 0 ? 1u : NSKINS - 1u)) % NSKINS);
            changed();                   /* (settings_apply: skin_apply) */
            app_say("SKIN", SKIN_NAME[skin_idx()]);
            ui.redraw = RG_ALL;
        } else if (i == 1) {             /* (0.9.57, FLEXOR: K2 KICK EQ, every kick's EQ on (right) or left out, flat
                                          * (left); the EQs untouched, nothing saved: ON at every start. K2 WAVE ZOOM
                                          * until 0.9.1: HOME's SETTINGS) */
            keq_flat = d < 0 ? 1u : 0u;
            kick_apply();
            app_say("KICK EQ", keq_flat ? "OFF (FLAT, ALL KICKS)" : "ON");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
            return;
        } else if (i == 2) {             /* K3 MODE: right EXPERT, left BEGINNER (left again, the TUTORIAL; right from
                                          * it, BEGINNER) */
            if (d < 0 && set.novice && !tut_on())
                tut_start();
            else if (d > 0 && tut_on()) {
                tut_quit();
                app_say("MODE", "BEGINNER");
            } else {
                set.novice = d > 0 ? 0u : 1u;
                if (set.novice) {
                    ui.meter_on = 0u;    /* (the METER: EXPERT's) */
                    ui.fx_page = 0u;     /* (the FX page: MAIN only) */
                    if (ui.fx_focus >= FX_EASY_SLOTS && ui.fx_focus < BRK_FX_SLOTS)
                        ui.fx_focus = 0u;   /* (0.229: the chain's two slots) */
                    if (ui.fx_main >= FX_EASY_SLOTS)
                        ui.fx_main = 0u;
                    novice_lengths();    /* (0.227, FLEXOR: the kick 32 steps, the drums 16; 0.246: all 16) */
                }
                changed();
                app_say("MODE", set.novice ? "BEGINNER" : "EXPERT");
            }
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else {                         /* K4 AUTO SAVE: right on, left off */
            set.auto_save = d > 0 ? 1u : 0u;
            changed();
            app_say("AUTO SAVE", set.auto_save ? "ON" : "OFF");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        }
        return;
    }
    if (ui.gpage) {
        if (i == 0)
            set.pitch_keep = d > 0 ? 1u : 2u;   /* right KEEP LENGTH, left SPEED */
        else if (i == 1)
            set.grain_ms = (uint8_t)clamp((int32_t)(set.grain_ms ? set.grain_ms : 30u) + d * 5, 10, 80);
        else if (i == 2)
            set.snap = (uint8_t)(clamp((int32_t)snap_index() + (d > 0 ? 1 : -1), 0, SNAPS - 1) + 1);
        else                             /* (LOAF BREAKER: K4 the break's LEVEL) */
            set.brk_level = (uint8_t)clamp((int32_t)set.brk_level + d * 2, 0, 100);
        global_value(i, t);
        app_say(i == 0 ? "PITCH" : i == 1 ? "GRAIN" : i == 2 ? "SNAP" : "BREAK LEVEL", i == 0 && pitch_keeps()
                ? "KEEP LENGTH" : i == 2 ? SNAP_NAME[snap_index()] : t);
        changed();
        return;
    }
    if (i == 0) {
        set.clk_in = d > 0;
        if (!set.clk_in) {               /* the tempo is the knob's again */
            ext.i = ext.n = 0u;
            ext.run = 0u;
            ext.bpm = ext.used = 0.0f;
        }
    } else if (i == 1)
        set.clk_out = d > 0;
    else if (i == 2 && !set.novice) {    /* LOAF BREAKER: K3 the METER, EXPERT only (0.213; SETUP's before): right on,
                                          * left off (not saved): CPU, IMG, FR in the header */
        ui.meter_on = d > 0 ? 1u : 0u;
        app_say("METER", ui.meter_on ? "ON: CPU, IMG, FR" : "OFF");
        ui.redraw |= RG_HEAD | RG_FOOT;
        return;
    } else                               /* (K4 REC KEY 0.9.58-0.9.69: gone, FLEXOR: REC set on ARP › BLACK alone) */
        return;
    global_value(i, t);
    app_say(NAME[i], t);
    changed();
}

/* the GLOBAL page's panel: Bluetooth, the clock coming in, the clock going out, MIDI through each port */
/* LOAF BREAKER (0.9.30, FLEXOR): GLO EFFECTS' cards, two rows of four on rows 46-189, drawn into the strip from screen
 * row top: each effect's card a small picture (a block, a zigzag, a floor) with that effect done to it, its key and
 * name below, lit when on; the eighth what the keys do */
static void sfx_cards(int32_t top)
{
    uint32_t f, cur = ui.sfx_cur < SFX_CRT || ui.sfx_cur >= SFX_N ? SFX_CRT : ui.sfx_cur;
    cv_oy = -top;
    for (f = SFX_CRT; f <= SFX_N; f++) {
        int32_t x = 4 + (int32_t)((f - 1u) % 4u) * 59, y = 46 + (int32_t)((f - 1u) / 4u) * 74, i, yy;
        char n[12] = {(char)('0' + f), ' ', 0};
        if (f == SFX_N) {                /* (the last place: what the keys do here) */
            cv_text(x + 2, y + 14, &FONT_XS, "KEYS 1-7:", C_DIM);
            cv_text(x + 2, y + 28, &FONT_XS, "ON / OFF", C_DIM);
            cv_text(x + 2, y + 42, &FONT_XS, "K1: PICK", C_DIM);
            break;
        }
        if (f == cur)
            cv_rect(x - 2, y - 2, 59, 74, C_WHITE);
        cv_rect(x, y, 55, 70, C_BLACK);
        cv_rect(x + 1, y + 1, 53, 50, C_LINE);       /* the picture: a sky, a floor, a block, a zigzag */
        cv_rect(x + 1, y + 38, 53, 13, C_DIM);
        cv_rect(x + 8, y + 12, 14, 14, C_HI);
        for (i = 0; i < 53; i++) {
            yy = (i * 3) % 24;
            cv_rect(x + 1 + i, y + 20 + (yy < 12 ? yy : 24 - yy), 1, 2, C_WHITE);
        }
        for (yy = y + 1; yy < y + 51; yy++) {   /* (the effect, on this strip's rows of it) */
            int32_t r = yy - top;
            uint16_t *p, tmp[53];
            if (r < 0 || r >= (int32_t)cv_h)
                continue;
            p = cv_px + (uint32_t)r * cv_w + (uint32_t)(x + 1);
            for (i = 52; i >= 0; i--) {
                uint32_t c = swap16(p[i]), h = ((uint32_t)i * 2654435761u) ^ ((uint32_t)yy * 40503u);
                h ^= h >> 13;
                if (f == SFX_CRT && (yy & 1))
                    c = (c >> 2) & 0x39E7u;
                else if (f == SFX_GRAIN && (h & 6u) == 0u)
                    c = h & 1u ? (c >> 1) & 0x7BEFu : c | 0x39E7u;
                else if (f == SFX_RGB)
                    c = (c & 0x07FFu) | (i >= 3 ? swap16(p[i - 3]) & 0xF800u : 0u);
                else if (f == SFX_SHAKE && i >= 5)
                    c = ((c >> 1) & 0x7BEFu) + ((swap16(p[i - 5]) >> 1) & 0x7BEFu);
                else if (f == SFX_STROBE)
                    c = ~c & 0xFFFFu;
                else if (f == SFX_8BIT)
                    c = swap16(p[i - i % 3]) & 0xC618u;
                tmp[i] = swap16(c);
            }
            if (f == SFX_VHS && ((yy - y) / 5) % 3 == 1) {   /* (a band in three slipped aside) */
                int32_t d = ((yy - y) / 15) & 1 ? 5 : -5;
                for (i = 0; i < 53; i++)
                    p[i] = tmp[(i - d + 53) % 53];
            } else
                for (i = 0; i < 53; i++)
                    p[i] = tmp[i];
            if (f == SFX_8BIT && (yy - y - 1) % 3 && r > 0)
                for (i = 0; i < 53; i++)
                    p[i] = p[i - (int32_t)cv_w];   /* (3 x 3 blocks, the row above's) */
        }
        cv_rect(x, y + 53, 55, 17, SFX_ON(f) ? C_HI : C_BLOCK);
        text_cat(n, SFX_NAME[f]);
        cv_text(x + 3, y + 55, text_w(&FONT_T, n) <= 50 ? &FONT_T : &FONT_XS, n, SFX_ON(f) ? C_BLACK : C_GRAY);
    }
    cv_oy = 0;
}
static void draw_global(const brk_status_t *st)
{
    static const char *const PORT_NAME[NPORTS] = {"USB", "BT", "TRS"};
    char t[32], v[16];
    uint32_t p;
    int bs = plat_ble_state();
    cv_begin(240, 44, C_BLACK);          /* LOAF BREAKER: no waveform on GLOBAL: its place the skins' on MORE, black
                                          * on the others */
    if (ui.gpage == GP_FILL) {           /* LOAF BREAKER: GROOVE: the swing (0.244: FILL gone), and the bar's 16ths
                                          * as it lays them out */
        static const char *const SN[3] = {"SWING", "BREAK ROWS", "FOLLOW"};
        uint32_t s;
        char v[12];
        cv_text(6, 1, &FONT_T, "GROOVE", C_GRAY);   /* (0.9.31-0.9.67 SEQ SETTINGS; 0.9.68, FLEXOR: GROOVE, its tab's name) */
        cv_text(6, 16, &FONT_XS, "K1 SWING  K2 THE BREAK'S ROWS IN FULL", C_DIM);
        cv_text(6, 28, &FONT_XS, "K3 FOLLOW: THE GRID ON THE BAR PLAYING", C_DIM);
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        for (s = 0; s < 3u; s++) {       /* (each setting's box, its value large) */
            global_value((int)s, v);
            cv_rect(4 + (int32_t)s * 78, 4, 74, 42, C_BLOCK);
            cv_text(8 + (int32_t)s * 78, 6, &FONT_XS, SN[s], C_GRAY);
            cv_text(8 + (int32_t)s * 78, 24, text_w(&FONT_B, v) <= 66 ? &FONT_B : &FONT_T, v, C_WHITE);
        }
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_XS, "THE BAR'S 16THS, SWUNG", C_DIM);
        for (s = 0; s < 16u; s++) {      /* (the odd 16ths pushed later as the swing grows) */
            int32_t x = 6 + (int32_t)s * 14 + ((s & 1u) ? (int32_t)(set.swing - 50u) * 6 / 25 : 0);
            cv_rect(x, 18, 10, 22, (s & 3u) ? C_BLOCK : C_HI);
        }
        cv_blit(0, 140);
        return;
    }
    if (ui.gpage == GP_LIGHTS) {         /* (0.9.57: LIGHTS: what they do, the FM-1's lights as they are now, the four
                                          * settings) */
        static const uint8_t BK_AFTER[11] = {0u, 1u, 2u, 4u, 5u, 7u, 8u, 9u, 11u, 12u, 14u};   /* (F3 up: the black keys
                                          * between the white ones) */
        static const char *const LN[4] = {"GLOW", "PULSE", "ON", "STYLE"};
        uint32_t j;
        char v[16];
        cv_text(6, 1, &FONT_T, "LIGHTS", C_GRAY);
        cv_text(6, 16, &FONT_XS, "K1 GLOW: THE LIGHTS AT REST (LIT: FULL)", C_DIM);
        cv_text(6, 28, &FONT_XS, "K2-K4: THE PULSE, ON THE TEMPO, PLAYING", C_DIM);
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        for (j = 0; j < NB; j++) {       /* (the buttons, then the keys: lit, mid, glowing, off) */
            uint32_t m = 1u << j;
            cv_rect(8 + (int32_t)j * 16, 2, 12, 6, lt_lb & m ? C_HI : lt_mb & m ? C_GRAY : lt_db & m ? C_DIM : C_LINE);
        }
        for (j = 0; j < 16u; j++) {
            uint32_t m = 1u << WK[j];
            cv_rect(8 + (int32_t)j * 14, 30, 12, 18, lt_lk & m ? C_HI : lt_mk & m ? C_GRAY : lt_dk & m ? C_DIM : C_LINE);
        }
        for (j = 0; j < 11u; j++) {
            uint32_t m = 1u << BK[j];
            cv_rect(15 + (int32_t)BK_AFTER[j] * 14, 12, 12, 14, lt_lk & m ? C_HI : lt_mk & m ? C_GRAY : lt_dk & m ? C_DIM : C_LINE);
        }
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        for (j = 0; j < 4u; j++) {       /* (each setting's box, its value large) */
            lights_value((int)j, v);
            cv_rect(4 + (int32_t)j * 59, 4, 55, 42, C_BLOCK);
            cv_text(8 + (int32_t)j * 59, 6, &FONT_XS, LN[j], C_GRAY);
            cv_text(8 + (int32_t)j * 59, 24, text_w(&FONT_B, v) <= 48 ? &FONT_B : &FONT_T, v,
                    j && !led_cfg(1u) ? C_DIM : C_WHITE);
        }
        cv_blit(0, 140);
        return;
    }
    if (ui.gpage == 3u) {                /* LOAF BREAKER: EFFECTS: the seven, lit when on (keys 1-7), K1's outlined;
                                          * 0.9.30, FLEXOR: bigger, each one's picture with its effect on it */
        static const uint8_t TOP[3] = {44u, 92u, 140u}, HT[3] = {48u, 48u, 52u};
        uint32_t b;
        for (b = 0; b < 3u; b++) {
            cv_begin(240, HT[b], C_BLACK);
            sfx_cards(TOP[b]);
            cv_blit(0, TOP[b]);
        }
        return;
    }
    if (ui.gpage == 4u) {                /* LOAF BREAKER: FX SET: the effects' four settings as bars */
        static const char *const L[4] = {"AMOUNT", "ON A KICK", "AT THE DROP", "LENGTH"};
        uint32_t k;
        cv_text(6, 1, &FONT_T, "EFFECTS SET", C_GRAY);
        cv_text(96, 16, &FONT_XS, "FOR ALL THE EFFECTS ON", C_DIM);
        cv_text(96, 28, &FONT_XS, "KICK, DROP: 0 = NONE", C_DIM);
        cv_blit(0, 44);
        for (k = 0; k < 2u; k++) {
            uint32_t j;
            cv_begin(240, 52, C_BLACK);
            for (j = 0; j < 2u; j++) {
                uint32_t n = k * 2u + j, v = *sfx_knob((int)n);
                char a[8];
                int32_t y = (int32_t)j * 26;
                cv_text(6, y + 2, &FONT_T, L[n], C_GRAY);
                pct_text((float)v * 0.01f, a);
                cv_text(196, y + 2, &FONT_T, a, C_WHITE);
                cv_rect(96, y + 7, 94, 5, C_LINE);
                cv_rect(96, y + 7, (int32_t)(v * 94u / 100u), 5, C_HI);
            }
            cv_blit(0, 88u + k * 52u);
        }
        return;
    }
    if (ui.gpage == 2u) {                /* MORE: the skin, and every skin's colours to pick from (K4) */
        uint32_t s;
        cv_text(6, 1, &FONT_T, "K1 SKIN", C_GRAY);
        cv_text(96, 1, &FONT_T, SKIN_NAME[skin_idx()], C_HI);
        for (s = 0; s < NSKINS; s++) {   /* a swatch each: its value colour on its ground, the one on outlined */
            int32_t x = 6 + (int32_t)(s % 13u) * 18, y = 18 + (int32_t)(s / 13u) * 13;   /* (K-GLITCH 0.2b: 13 a row, RIBBON the 25th) */   /* (LOAF BREAKER, 0.237:
                                          * two rows of twelve, FLEXOR's) */
            const uint8_t (*c)[3] = SKIN_RGB[s];
            if (s == skin_idx())
                cv_rect(x - 2, y - 2, 20, 15, C_WHITE);
            cv_rect(x, y, 16, 11, skin_rgb(c[5], 100u));
            cv_rect(x + 2, y + 2, 12, 4, skin_rgb(c[4], 100u));
            cv_rect(x + 2, y + 7, 12, 2, skin_rgb(c[3], 100u));
        }
        cv_blit(0, 44);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "K2 KICK EQ", C_GRAY);   /* (0.9.57, FLEXOR: every kick's EQ, or none) */
        cv_text(96, 2, &FONT_T, keq_flat ? "OFF (FLAT)" : "ON", keq_flat ? C_AMB : C_HI);
        cv_text(96, 14, &FONT_XS, "ALL KICKS, ON AT START", C_DIM);
        cv_text(6, 38, &FONT_T, "K3 MODE", C_GRAY);
        cv_text(96, 38, &FONT_T, tut_on() ? "TUTORIAL" : set.novice ? "BEGINNER" : "EXPERT", C_HI);
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "K4 AUTO SAVE", C_GRAY);
        cv_text(96, 2, &FONT_T, set.auto_save ? "ON" : "OFF", set.auto_save ? C_HI : C_DIM);
        cv_text(96, 16, &FONT_XS, "A MINUTE AFTER A CHANGE,", C_DIM);
        cv_text(96, 28, &FONT_XS, "ONCE STOPPED 20 S", C_DIM);
        cv_blit(0, 140);
        return;
    }
    cv_blit(0, 44);
    if (ui.gpage) {                      /* the pitch: what the mode does, the pitch now */
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "PITCH", C_GRAY);
        cv_text(96, 2, &FONT_T, pitch_keeps() ? "KEEP LENGTH" : "SPEED", C_HI);
        cv_text(96, 16, &FONT_XS, pitch_keeps() ? "SLICES KEEP LENGTH" : "PITCH CHANGES LENGTH", C_DIM);
        cv_text(6, 30, &FONT_T, "NOW", C_GRAY);
        t[0] = set.pitch > 0 ? '+' : 0;
        t[1] = 0;
        int_text(set.pitch, t + (set.pitch > 0));
        text_cat(t, " ST  (OCT- / OCT+)");
        cv_text(96, 30, &FONT_T, t, C_WHITE);
        cv_blit(0, 88);
        cv_begin(240, 52, C_BLACK);
        cv_text(6, 2, &FONT_T, "GRAIN", C_GRAY);
        int_text(set.grain_ms ? set.grain_ms : 30, t);
        text_cat(t, " MS");
        cv_text(96, 2, &FONT_T, t, pitch_keeps() ? C_WHITE : C_DIM);
        cv_text(96, 14, &FONT_XS, "LOWER: TIGHTER HITS", C_DIM);
        cv_text(6, 30, &FONT_T, "SNAP", C_GRAY);   /* where a key lands, and what that is in this break */
        t[0] = 0;
        text_cat(t, SNAP_NAME[snap_index()]);
        if (snap_index()) {
            const brk_break_t *b = brk_get_break();
            text_cat(t, " = ");
            sixteenths_text(SNAP_SL[snap_index()] * key_sps(b), v);
            text_cat(t, v);
        }
        cv_text(96, 30, &FONT_T, t, C_HI);
        cv_blit(0, 140);
        return;
    }
    cv_begin(240, 52, C_BLACK);          /* where MIDI goes (LOAF BREAKER: no Bluetooth), and the clock in */
    (void)bs;
    cv_text(6, 2, &FONT_T, "MIDI", C_GRAY);
    cv_text(96, 2, &FONT_T, "USB AND THE TRS JACK", C_WHITE);
    cv_text(6, 16, &FONT_T, "NOTES IN", C_GRAY);   /* (0.9.64, FLEXOR: PRESETS, ALL / DRUMS / OFF) */
    cv_text(96, 16, &FONT_T, notes_in() == NI_ALL ? "ALL  (PRESETS)" : notes_in() == NI_DRUMS ? "DRUMS ONLY  (PRESETS)"
            : "OFF  (PRESETS)", notes_in() == NI_ALL ? C_WHITE : C_HI);
                                         /* (BREAK TIME gone, 0.9.25, FLEXOR: HOME's SETTINGS K4 alone) */
    cv_text(6, 30, &FONT_T, "CLOCK IN", C_GRAY);
    t[0] = 0;
    if (!set.clk_in)
        text_cat(t, "OFF");
    else if (!ext_active())
        text_cat(t, "WAITING FOR CLOCK");
    else {
        tenths_text((int32_t)(ext.bpm * 10.0f + 0.5f), v);
        text_cat(t, v);
        text_cat(t, " BPM ");
        text_cat(t, PORT_NAME[ext.port % NPORTS]);
        text_cat(t, ext.run ? "  RUNNING" : "  STOPPED");
    }
    cv_text(96, 30, &FONT_T, t, ext_active() ? C_HI : set.clk_in ? C_WHITE : C_DIM);
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);          /* the clock out, and MIDI through each port (lit: in the last 0.3 s) */
    cv_text(6, 2, &FONT_T, "CLOCK OUT", C_GRAY);
    t[0] = 0;
    if (!set.clk_out)
        text_cat(t, "OFF");
    else {
        text_cat(t, "TO USB");           /* (LOAF BREAKER, 0.259: no Bluetooth: USB, whatever MIDI TO says) */
        text_cat(t, st->playing ? "  SENDING" : "  (WHILE PLAYING)");
    }
    cv_text(96, 2, &FONT_T, t, set.clk_out && st->playing ? C_HI : set.clk_out ? C_WHITE : C_DIM);
    cv_text(6, 30, &FONT_T, "MIDI", C_GRAY);
    for (p = 0; p < NPORTS; p++) {       /* (LOAF BREAKER, 0.259: USB and TRS; no Bluetooth, no BT box) */
        int32_t x = 96 + (p == PORT_TRS ? 1 : 0) * 48;
        if (p != PORT_TRS && p != 0u)
            continue;
        int in = plat_midi_ago((int)p, 0) < 300u, out = p != PORT_TRS && plat_midi_ago((int)p, 1) < 300u;
        cv_rect(x, 28, 44, 18, C_BLOCK);
        cv_text(x + 4, 30, &FONT_T, PORT_NAME[p], in || out ? C_WHITE : C_DIM);
        cv_rect(x + 34, 31, 4, 4, in ? C_HI : C_LINE);       /* in */
        cv_rect(x + 34, 38, 4, 4, out ? C_HI : C_LINE);      /* out */
    }
    cv_blit(0, 140);
}

static void global_part(uint32_t g)      /* GLO (again: the next part) and ALGORITHM on GLOBAL (LOAF BREAKER: and the
                                          * break's page, GROOVE) */
{
    if (g == GLO_HSET) {                 /* (0.9.56: SYSTEM's SETTINGS, HOME's before) */
        ui.page = PG_HSET;
        ui.redraw |= RG_FOOT | RG_BLOCKS;
        page_say();
        return;
    }
    ui.gpage = (uint8_t)g;
    ui.page = PG_GLOBAL;
    ui.redraw |= RG_FOOT | RG_BLOCKS;
    page_say();
}

/* LOAF BREAKER (0.9.2, FLEXOR): HOME's SETTINGS, the waveform's settings where it is played: K1 VIEW (the waveform or
 * the slices; HOME held still flips it), K2 WAVE TYPE (REKORDBOX, SERATO; SYSTEM EFFECTS' K3 until 0.9.24), K3 WAVE ZOOM (SYSTEM
 * SETUP's K2 until 0.9.1), K4 BREAK TIME (SYSTEM MIDI's K4 until 0.9.1). VIEW saved in bkeys.bk[BK_VIEW] */
static const char *const HSET_NAME[4] = {"VIEW", "WAVE TYPE", "WAVE ZOOM", "BREAK TIME"};
static void hset_value(int i, char *t)
{
    t[0] = 0;
    text_cat(t, i == 0 ? (ui.blocks ? "SLICES" : "WAVEFORM") : i == 1 ? WS_NAME[ws_style()]   /* (0.9.70: 3 BAND, BLUE) */
                : i == 2 ? CDJ_ZNAME[set.wzoom % CDJ_ZN] : (set.bkeys.bk[BK_BHALF] & 1u ? "HALF" : "NORMAL"));
}
static void hset_turn(int i, int32_t d)
{
    char t[16];
    if (i == 0) {
        ui.blocks = d > 0 ? 1u : 0u;     /* (right the slices, left the waveform) */
        set.bkeys.bk[BK_VIEW] = ui.blocks;
    } else if (i == 1)                   /* (0.9.70, FLEXOR: RGB, SERATO, 3 BAND, BLUE) */
        set.wstyle = (uint8_t)clamp((int32_t)ws_style() + (d > 0 ? 1 : d < -1 ? (int32_t)d : -1), 0, WS_N - 1);   /* (the Studio: -3 back to RGB) */
    else if (i == 2)
        set.wzoom = (uint8_t)clamp((int32_t)(set.wzoom % CDJ_ZN) + (d > 0 ? 1 : -1), 0, CDJ_ZN - 1);
    else
        set.bkeys.bk[BK_BHALF] = d > 0 ? 1u : 0u;   /* (right HALF: a 115 break under a 230 kick at its own speed) */
    changed();
    hset_value(i, t);
    app_say(HSET_NAME[i & 3], t);
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
static void strip_turn(int32_t d)        /* (0.9.70, FLEXOR) SYSTEM › DISPLAY's PRESETS: the strip's style, round */
{
    strip_put((strip_style() + (d > 0 ? 1u : SS_N - 1u)) % SS_N);
    set_dirty = 1u;
    app_say("STRIP", SS_NAME[strip_style()]);
    ui.redraw |= RG_ALL;
}
static void break_ask(uint32_t i);
/* LOAF BREAKER (0.9.71, FLEXOR, from CHOPPAH 1.5, PRD S-030 to S-033): the split, HOME's third part (EXPERT): K1 on /
 * off, K2 break B, K3 A's bar, K4 B's bar; a change reloads the break at the next bar, as a break change */
static void split_value(int i, char *t)
{
    t[0] = 0;
    if (i == 0)
        text_cat(t, split_on() ? "ON" : "OFF");
    else if (i == 1)
        text_cat(t, break_name(split_b_of(set.brk)));
    else
        text_cat(t, split_bars_of(set.brk) >> (i - 2) & 1u ? "BAR 2" : "BAR 1");
}
static void split_turn(int i, int32_t d)
{
    static const char *const SN[4] = {"SPLIT", "BREAK B", "BAR OF A", "BAR OF B"};
    uint32_t was = sp_get(set.brk), v = was;   /* (0.9.71, FLEXOR: the break playing's own split, kept for it) */
    char t[16];
    if (set.novice) {
        easy_no();
        return;
    }
    if (set.brk >= WK_NB) {
        app_say("SPLIT", "NOT ON THIS BREAK");
        return;
    }
    if (!(v & SP_BMASK))                 /* (B as shown: the one after A, until one is picked) */
        v |= split_b_of(set.brk) + 1u;
    if (i == 0)
        v = d > 0 ? v | SP_ON : v & ~SP_ON;
    else if (i == 1)
        v = (v & ~SP_BMASK) | ((split_b_of(set.brk) + (d > 0 ? 1u : n_breaks - 1u)) % n_breaks + 1u);
    else if (d > 0)
        v |= 1u << (SP_BARS + (uint32_t)i - 2u);
    else
        v &= ~(1u << (SP_BARS + (uint32_t)i - 2u));
    if (was != v) {
        sp_put(set.brk, v);
        if (!(v & SP_ON))
            brk_half(-1);
        if ((v & SP_ON) || i == 0)       /* the break again, at the next bar (break_poll), with the split's own keys
                                          * or the break's (break_ask's wk_follow) */
            break_ask(set.brk);
        set_dirty = 1u;
    }
    split_value(i, t);
    app_say(SN[i & 3], t);               /* (after break_ask's own) */
    ui.redraw |= RG_BLOCKS | RG_HEAD;
}
/* LOAF BREAKER (0.9.71): HOME's SPLIT, the four as cards (the name, the value), then the two breaks and how the keys
 * work */
static void draw_split(void)
{
    const brk_break_t *b = brk_get_break();
    static const char *const N[4] = {"SPLIT", "BREAK B", "BAR A", "BAR B"};
    uint32_t i;
    char t[24];
    int on = (int)split_on();
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        cv_rect(x, 0, 56, 50, C_BLOCK);
        cv_text(x + 4, 3, &FONT_XS, N[i], C_GRAY);
        split_value((int)i, t);
        if (text_w(&FONT_T, t) > 50) {   /* (a break's name: its first word, on the card) */
            uint32_t j = 0u;
            while (t[j] && t[j] != ' ')
                j++;
            t[j] = 0;
        }
        cv_text(x + 4, 32, text_w(&FONT_T, t) > 50 ? &FONT_XS : &FONT_T, t, i == 0 || on ? C_HI : C_DIM);
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    t[0] = 0;
    text_cat(t, "A ");
    text_cat(t, break_name(set.brk));
    cv_text(6, 2, &FONT_XS, t, on ? C_WHITE : C_DIM);
    cv_text(130, 2, &FONT_XS, split_bars_of(set.brk) & 1u ? "BAR 2: KEYS 1-8" : "BAR 1: KEYS 1-8", C_DIM);
    t[0] = 0;
    text_cat(t, "B ");
    text_cat(t, break_name(split_b_of(set.brk)));
    cv_text(6, 13, &FONT_XS, t, on ? C_WHITE : C_DIM);
    cv_text(130, 13, &FONT_XS, split_bars_of(set.brk) & 2u ? "BAR 2: KEYS 9-16" : "BAR 1: KEYS 9-16", C_DIM);
    cv_text(6, 26, &FONT_XS, !on ? "K1: ONE BAR OF A, ONE OF B, IN A'S TEMPO" : b && b->split ?
            "KEYS PLAY THEIR SLICES, THE LOOP A + B" : "ON AT THE NEXT BAR", on ? C_AMB : C_DIM);
    cv_text(6, 37, &FONT_XS, "A KEY OF EACH HELD: LAYERED", on ? C_WHITE : C_DIM);
    cv_blit(0, 140);
}
/* LOAF BREAKER (0.9.75, FLEXOR: "une fonction un peu mathcore lite ... scramble ... pour rajouter du chaos", the
 * break): HOME › CHAOS, after GENERATOR (and SPLIT in EXPERT). K1 CHAOS (each hit for another of its role: a kick for a
 * kick, a snare for a snare; backwards from 50), K2 MATH (OFF, 3, 5, 7, 11, 13: each bar's first N 16ths round against
 * the bar), K3 ROLLS (snares in three, hats in two), K4 SEED (the variation: the same seed, the same chaos every loop).
 * The roles: CHOPPAH's Slice Map, heard (brk.c brk_auto_roles); in at the next step */
static const uint8_t CH_MATH[6] = {0u, 3u, 5u, 7u, 11u, 13u};
static uint32_t chaos_math_idx(void)
{
    uint32_t i;
    for (i = 0; i < 6u; i++)
        if (kx2.chaos[1] == CH_MATH[i])
            return i;
    return 0u;
}
static void chaos_value(int i, char *t)
{
    t[0] = 0;
    if (i == 1) {
        if (!kx2.chaos[1])
            text_cat(t, "OFF");
        else
            int_text(kx2.chaos[1], t);
    } else if ((i == 0 || i == 2) && !kx2.chaos[i])
        text_cat(t, "OFF");
    else
        int_text(kx2.chaos[i & 3], t);
}
static void chaos_turn(int i, int32_t d)
{
    static const char *const CN[4] = {"CHAOS", "MATH", "ROLLS", "SEED"};
    char t[16];
    kx2.ch_tag = CH_TAG;
    kx2.ch_sw = CH_SW;                   /* (0.9.76: a knob turned here, CHAOS heard again: the CHS key's switch on) */
    if (i == 1)
        kx2.chaos[1] = CH_MATH[(uint32_t)clamp((int32_t)chaos_math_idx() + (d > 0 ? 1 : -1), 0, 5)];
    else if (i == 3)
        kx2.chaos[3] = (uint8_t)clamp((int32_t)kx2.chaos[3] + d, 0, 99);
    else
        kx2.chaos[i] = (uint8_t)clamp((int32_t)kx2.chaos[i] + d * 2, 0, 100);
    chaos_push();
    set_dirty = 1u;
    chaos_value(i, t);
    app_say(CN[i & 3], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* the four as cards, then the break's slices with what the Slice Map heard in each (K kick, S snare, H hat, P perc) */
static void draw_chaos(void)
{
    const brk_break_t *b = brk_get_break();
    static const char *const N[4] = {"CHAOS", "MATH", "ROLLS", "SEED"};
    uint32_t i, n;
    char t[16];
    int on = (kx2.chaos[0] || kx2.chaos[1] || kx2.chaos[2]) && !chaos_off;
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        cv_rect(x, 0, 56, 50, C_BLOCK);
        cv_text(x + 4, 3, &FONT_XS, N[i], C_GRAY);
        chaos_value((int)i, t);
        cv_text(x + 4, 32, &FONT_T, t, i == 3 ? (on ? C_WHITE : C_DIM) : kx2.chaos[i] ? C_HI : C_DIM);
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    n = b ? (b->nslices < BRK_MAX_SLICES ? b->nslices : BRK_MAX_SLICES) : 0u;
    for (i = 0; i < n; i++) {
        uint32_t r = b->roles[i];
        int32_t x0 = 2 + (int32_t)(i * 236u / n), x1 = 2 + (int32_t)((i + 1u) * 236u / n) - 2;
        const char *l = r & 1u << BRK_ROLE_KICK ? "K" : r & 1u << BRK_ROLE_SNARE ? "S" : r & 1u << BRK_ROLE_HAT ? "H" :
                        r & 1u << BRK_ROLE_PERC ? "P" : "";
        uint16_t c = r & 1u << BRK_ROLE_KICK ? C_WHITE : r & 1u << BRK_ROLE_SNARE ? C_AMB : r ? C_GRAY : C_BLOCK;
        cv_rect(x0, 2, x1 - x0, 14, r ? C_BLOCK : C_BLACK);
        cv_rect(x0, 14, x1 - x0, 2, c);
        if (l[0] && x1 - x0 >= 6)
            cv_text(x0 + (x1 - x0 - text_w(&FONT_XS, l)) / 2, 4, &FONT_XS, l, c);
    }
    cv_text(6, 24, &FONT_XS, on ? "SCRAMBLED BY ROLE, SAME EVERY LOOP" : chaos_off ? "OFF BY THE CHS KEY: A KNOB, ON" :
            "K1: EACH HIT SWAPPED FOR ITS ROLE", on ? C_AMB : C_DIM);
    cv_text(6, 36, &FONT_XS, "K4 SEED: ANOTHER VARIATION", C_DIM);
    cv_blit(0, 140);
}
static void draw_hset(void)
{
    uint32_t i;
    char t[48], v[16];
    cv_begin(240, 52, C_BLACK);          /* the four, as cards: the name, the value large (K1-K4 left to right) */
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        cv_rect(x, 0, 56, 50, C_BLOCK);
        {                                /* (the name a word a line: the card's width) */
            const char *n = HSET_NAME[i];
            uint32_t j = 0u;
            while (n[j] && n[j] != ' ')
                j++;
            t[0] = 0;
            text_cat(t, n);
            t[j] = 0;
            cv_text(x + 4, 3, &FONT_XS, t, C_GRAY);
            if (n[j])
                cv_text(x + 4, 13, &FONT_XS, n + j + 1, C_GRAY);
        }
        hset_value((int)i, t);
        cv_text(x + 4, 32, text_w(&FONT_T, t) > 50 ? &FONT_XS : &FONT_T, t, C_HI);
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);          /* what they do */
    t[0] = 0;
    text_cat(t, set.bkeys.bk[BK_BHALF] & 1u ? "THE BREAK AT HALF: " : "THE BREAK AT THE TEMPO: ");
    tenths_text((int32_t)set.tempo_x10 / ((set.bkeys.bk[BK_BHALF] & 1u) ? 2 : 1), v);
    text_cat(t, v);
    text_cat(t, " BPM");
    cv_text(6, 2, &FONT_XS, "HOME HELD: WAVEFORM / SLICES", C_DIM);
    {                                    /* (0.9.70, FLEXOR: PRESETS, the strip's style) */
        char sv[32] = "PRESETS: THE STRIP ";
        text_cat(sv, SS_NAME[strip_style()]);
        cv_text(6, 14, &FONT_XS, sv, C_HI);
    }
    cv_text(6, 26, &FONT_XS, t, C_DIM);
    cv_text(6, 38, &FONT_XS, "SELECT: SYSTEM'S PARTS", C_DIM);
    cv_blit(0, 140);
}
