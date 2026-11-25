/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the MIXER (LFO): the break, the kick, the snare and hat, FX; swing and the drums' route. (Part of
 * app.c's unity: included there in order.) */

/* the MIXER's knob i on half h, as the foot and the strip show it */
static void selk_value(uint32_t f, int i, char *t);
static void mix_value(uint32_t h, int i, char *t)
{
    t[0] = 0;
    if (h == 3u) {                       /* LOAF BREAKER: DYNAMICS: COMP's attack, release; DUCK's release, what it ducks */
        selk_value(i < 2 ? 2u : 0u, i == 0 ? 2 : i == 1 ? 3 : i == 2 ? 2 : 3, t);
        return;
    }
    if (h == 2u) {                       /* LOAF BREAKER: MASTER: COMP, its RATIO, DUCK, MAX OUT */
        if (i < 3)
            selk_value(i == 2 ? 0u : 2u, i == 2 ? 0 : i, t);
        else {
            uint32_t db = max_out_db();
            if (db)
                text_cat(t, "-");
            int_text((int32_t)db, t + (db ? 1 : 0));
            text_cat(t, " DB");
        }
        return;
    }
    if (h) {                             /* LOAF BREAKER: SENDS: FX, the drums' route (SWING: PATTERN's GROOVE) */
        if (i == 0)
            pct_text(set.fx.wet, t);
        else if (i == 1)
            text_cat(t, set.drums_chain ? "THRU FX" : "AROUND FX");
        else if (i == 2)
            selk_value(0u, 3, t);        /* (0.9.64, FLEXOR: DUCKS, BREAK / PERC / BOTH; nothing in 0.9.51-0.9.63) */
        else
            text_cat(t, ROLL_THRU_NAME[set.pads.rsv[0] % 3u]);   /* (0.9.40: the rolls' THRU) */
        return;
    }
    if (i == 0) {
        int_text(set.brk_level, t);
        text_cat(t, "%");
    } else if (i < 3)                    /* (K4: the MELODY's, 0.200-0.208; the ARP's route in 0.9.29-0.9.30, SENDS'
                                          * alone since, FLEXOR) */
        pct_text(i == 1 ? set.kick.volume : set.perc.volume, t);
}

/* the MIXER's strip: four channels side by side (a fader, its value; BREAK and KICK with their level meters, FX with
 * the chain's slots; their values are in the foot), drawn as one 240 x 104 picture in two 52-row strips. GROOVE:
 * swing, the drums' route */
#define MX_Y 18                          /* the faders and meters: from row 18, 80 rows (the values are in the foot) */
#define MX_H 80
static int32_t meter_px(float lv, int32_t h)   /* a level's height on an h-pixel meter: -48..0 dBFS */
{
    float db = lv > 0.0039f ? 6.0206f * brk_log2f(lv) : -48.0f;
    return (int32_t)(brk_clampf((db + 48.0f) * (1.0f / 48.0f), 0.0f, 1.0f) * (float)h + 0.5f);
}
static void mix_meter(int32_t x, float lv)
{
    int32_t h = meter_px(lv, MX_H);
    cv_rect(x, MX_Y, 5, MX_H, C_BLOCK);
    if (h)
        cv_rect(x, MX_Y + MX_H - h, 5, h, h >= MX_H - 2 ? C_WHITE : h >= MX_H * 5 / 6 ? C_AMB : C_HI);
}
static void mix_fader(int32_t x, float v, int on)   /* 0..1 from the bottom */
{
    int32_t h = (int32_t)(brk_clampf(v, 0.0f, 1.0f) * (float)MX_H + 0.5f);
    cv_rect(x, MX_Y, 14, MX_H, C_BLOCK);
    if (h)
        cv_rect(x + 2, MX_Y + MX_H - h, 10, h, on ? C_HI : C_DIM);
    cv_rect(x - 1, MX_Y + MX_H - h - 1, 16, 2, C_WHITE);   /* the cap */
}
static void mix_picture(int32_t top)
{
    static const char *const NAME[4][4] = {{"BREAK", "KICK", "PERC", ""}, {"FX", "DRUMS", "DUCKS", "ROLL"},
                                           {"COMP", "RATIO", "DUCK", "MAX OUT"},
                                           {"C ATTACK", "C RELEASE", "D RELEASE", "DUCKS"}};
    uint32_t c;
    cv_oy = -top;                        /* (this strip's rows of the picture) */
    for (c = 0; c < 4u; c++) {
        int32_t x = (int32_t)c * 60;
        if (c)
            cv_rect(x, 4, 1, 96, C_LINE);
        if (NAME[ui.mpage % 4u][c][0])
            cv_text(x + 6, 1, &FONT_XS, NAME[ui.mpage % 4u][c], C_GRAY);
    }
    if (ui.mpage == 3u) {                /* DYNAMICS: the four values, large */
        char t[12];
        for (c = 0; c < 4u; c++) {
            mix_value(3u, (int)c, t);
            cv_text((int32_t)c * 60 + 6, 40, text_w(&FONT_B, t) <= 52 ? &FONT_B : &FONT_XS, t, C_WHITE);
        }
        return;
    }
    if (ui.mpage == 2u) {                /* MASTER: COMP and its reduction, its ratio, DUCK, MAX OUT */
        char t[8];
        mix_fader(10, (float)set.comp * 0.01f, set.comp > 0u);
        mix_meter(30, set.comp ? brk_exp2f(-comp_gr * 0.16610f) : 0.0f);   /* (the gain it leaves: 0 dB full) */
        mix_value(2u, 1, t);
        cv_text(72, 50, &FONT_B, t, set.comp ? C_WHITE : C_DIM);
        mix_fader(130, (float)set.duck * 0.01f, set.duck > 0u);
        mix_fader(190, (float)(12u - max_out_db()) * (1.0f / 12.0f), 1);
        return;
    }
    if (ui.mpage) {                      /* LOAF BREAKER: SENDS: FX, the chain's slots beside it; the drums' route */
        mix_fader(10, set.fx.wet, set.fx.wet > 0.005f);
        for (c = 0; c < fx_nslots(); c++) {   /* (LOAF BREAKER: BEGINNER's two) */
            const brk_fxslot_t *sl = &set.fx.slot[c];
            int on = sl->on && sl->type != FXT_EMPTY;
            cv_rect(30, MX_Y + (int32_t)c * 21, 6, 17, on ? C_BLOCK_HI : sl->type == FXT_EMPTY ? C_BLACK : C_BLOCK);
            if (sl->type == FXT_EMPTY)
                cv_rect(30, MX_Y + (int32_t)c * 21 + 8, 6, 1, C_LINE);
        }
        {                                /* the drums' route: two boxes, the one taken lit */
            int thru = set.drums_chain;
            cv_rect(66, 26, 48, 26, thru ? C_BLOCK_HI : C_BLOCK);
            cv_text(70, 31, &FONT_XS, "THRU FX", thru ? C_BLACK : C_GRAY);
            cv_rect(66, 62, 48, 26, thru ? C_BLOCK : C_BLOCK_HI);
            cv_text(70, 67, &FONT_XS, "AROUND", thru ? C_GRAY : C_BLACK);
        }
        {                                /* (0.9.64, FLEXOR: what DUCK ducks, the one taken lit; DUCK itself off: dim) */
            static const char *const DN[3] = {"BREAK", "PERC", "BOTH"};
            static const uint8_t DC[3] = {0u, 2u, 1u};
            uint32_t j;
            for (j = 0; j < 3u; j++) {
                int on = set.duck_what % 3u == DC[j];
                cv_rect(126, 22 + (int32_t)j * 26, 48, 22, on ? (set.duck ? C_BLOCK_HI : C_LINE) : C_BLOCK);
                cv_text(130, 27 + (int32_t)j * 26, &FONT_XS, DN[j], on ? C_BLACK : C_GRAY);
            }
        }
        {                                /* (0.9.40, FLEXOR: the three rolls' THRU, the one taken lit) */
            uint32_t j;
            for (j = 0; j < 3u; j++) {
                int on = set.pads.rsv[0] % 3u == j;
                cv_rect(186, 22 + (int32_t)j * 26, 48, 22, on ? C_BLOCK_HI : C_BLOCK);
                cv_text(190, 27 + (int32_t)j * 26, &FONT_XS, ROLL_THRU_NAME[j], on ? C_BLACK : C_GRAY);
            }
        }
        return;
    }
    mix_fader(10, (float)set.brk_level * 0.01f, set.brk_level > 0u);
    mix_meter(30, meter_brk);
    mix_fader(70, set.kick.volume, set.kick.volume > 0.0f);
    mix_meter(90, meter_kick);
    mix_fader(130, set.perc.volume, set.perc.volume > 0.0f);
    mix_meter(150, meter_perc);
}
static void draw_mixer(void)
{
    cv_begin(240, 52, C_BLACK);
    mix_picture(0);
    cv_oy = 0;
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    mix_picture(52);
    cv_oy = 0;
    cv_blit(0, 140);
}
