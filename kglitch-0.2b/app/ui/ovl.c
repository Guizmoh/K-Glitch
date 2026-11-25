/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the OVERLAY page (ARP, PRD B-52; LOAF BREAKER: gone 0.1xx-0.243 for FILL, back in 0.244, FLEXOR: "the screen
 * from before with all the FX"). Each white key an effect over everything that plays (dsp/ovl.c), on while held, any
 * together; with LATCH a press switches its effect on and off (and latched ones stay on on the other pages). K1 the
 * amount of the effect touched last, K2 the rate of the ones on the beat, K3 the sweeps' length, K4 LATCH. The ARP
 * black key shares the effect touched last (0.9.27, ovl_last), and the white keys of the effects on are lit (seq.c
 * keys_lit). (Part of app.c's unity: included there in order.) */

static uint32_t ovl_held, ovl_latched;
static uint32_t oct_ovl, kg_ovl;                 /* (K-GLITCH 0.2b: HOME, OCT+ held: the ARP effects of the white keys held, ui/kgchop.c) */   /* bit OV_*: held down on this page; switched on (LATCH) */
#define ovl_last (set.ovl_pad & 15u)     /* the effect touched last: K1 its amount (0.9.27, FLEXOR: the ARP key's own, one
                                          * effect for both: touched on the page, the key plays it; turned on the key's K1,
                                          * the page tunes it) */

static uint32_t ovl_pad_bits(void)       /* the ARP keys' effects on (0.9.45: each key its own) */
{
    uint32_t j, m = 0u;
    for (j = 0; j < 11u; j++)
        if (arpk_on(j))
            m |= 1u << arpk_fx(j);
    return m;
}
static void ovl_push(void)
{
    uint32_t on = ovl_held | ovl_latched | ovl_pad_bits() | oct_ovl | kg_ovl;
    ovl_set(on, set.ovl_amt, set.ovl_rate, set.ovl_sweep);
    kick_ovl = (uint8_t)(on >> OV_KX2 & 7u);   /* (0.9.55, FLEXOR: KICK X2, X4, TRI: the render's, as the black keys) */
}
static void ovl_pad_key(uint32_t j, int down)   /* GLOUPI BREAKS: the ARP black key j (0..10) down or up */
{
    uint16_t b = (uint16_t)(1u << (j % 11u));
    if (down) {                          /* (0.9.45: the key touched last: its knobs; the OVERLAY page's K1 its effect) */
        arp_foc = (uint8_t)(j % 11u);
        set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | arpk_fx(j));
    }
    if (arpk_latch(j)) {                 /* LOAF BREAKER: K4 LATCH on: each press turns it on, or off */
        if (down)
            arp_lat ^= b;
        arp_held &= (uint16_t)~b;
    } else {                             /* (off: on while held) */
        arp_held = (uint16_t)(down ? arp_held | b : arp_held & ~b);
        arp_lat &= (uint16_t)~b;
    }
    ovl_push();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}

static void ovl_keys(uint32_t keys, uint32_t kdn)   /* every input: the white keys on the page, then the engine */
{
    uint32_t i, held = 0u;
    if (ui.page == PG_OVL && ui.opage == 0u)   /* (ARP EFFECTS: the keys; BLACK KEYS, MORE: none) */
        for (i = 0; i < 16u; i++) {
            if (kdn >> WK[i] & 1u) {
                arpk_set_fx(arpk_focus(), i);   /* (0.9.45: the ARP key touched last plays it) */
                set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | i);
                set_dirty = 1u;
                tut_ovl++;               /* (LOAF BREAKER: the TUTORIAL's count) */
                if (set.ovl_latch)
                    ovl_latched ^= 1u << i;
                app_say(OVL_NAME[i], set.ovl_latch ? (ovl_latched >> i & 1u ? "ON" : "OFF") : "");
                ui.redraw |= RG_BLOCKS | RG_FOOT;   /* (K1: its amount) */
            }
            if (!set.ovl_latch && keys >> WK[i] & 1u)
                held |= 1u << i;
        }
    if (held != ovl_held)
        ui.redraw |= RG_BLOCKS;
    ovl_held = held;
    ovl_push();
}
/* ------------------------------------------------ BLACK KEYS (LOAF BREAKER) --- */
/* LOAF BREAKER (11 Nov 2026, FLEXOR): ARP's second tab, EXPERT's: the black keys' layout. K1 the PRESET (CHOPPAH,
 * FLEXOR LAYOUT, CUSTOM), K2 the KEY (or a black key pressed: it picks itself, nothing played), K3 its EFFECT (any of
 * CHOPPAH's and FLEXOR LAYOUT's, BKF_*; on a preset, K3 makes a CUSTOM of it first). (MORE, a third: gone for
 * now, 0.259, FLEXOR) */
static void bk_release_all(void)         /* (the layout changes: whatever a key had on lets go) */
{
    uint32_t i;
    for (i = 0; i < BRK_NPADS; i++)
        brk_pad((int)i, 0);
    roll_held = 0xFFu;
    roll_sel = 0u;
    kroll = 0u;
    half_lat = 0u;
    pad_latched = 0u;
    arp_held = arp_lat = 0u;
    ovl_push();
    kick_hold = 0u;
    ui.pad_focus = -1;
    ui.kil_focus = 0u;
    ui.x2_focus = 0u;
    kick_x2 = 0u;
}
static const char *bk_long(uint32_t j)   /* (0.9.70: the REC key with no function: it records) */
{
    return j == 11u && !rec_key_on() ? "NONE: REC RECORDS" : BKF_LONG[bk_fn(j)];
}
static const char *bk_short(uint32_t j) { return j == 11u && !rec_key_on() ? "-" : BKF_NAME[bk_fn(j)]; }
static void bk_key_name(uint32_t j, char *t)   /* ("KEY 3", the REC key "REC KEY", 0.9.58) */
{
    t[0] = 0;
    if (j == 11u) {
        text_cat(t, "REC KEY");
        return;
    }
    text_cat(t, "KEY ");
    int_text((int32_t)j % 11 + 1, t + 4);
}
#define FLAY_TAG 0x5Bu                    /* (kx2.rsv[52]: FLEXOR LAYOUT's key 5 given RISE, 0.9.58) */
static void flexor_lay_fix(int force)    /* (0.9.58, FLEXOR: FLEXOR LAYOUT's key 5, an ARP key, RISE: once on an FM-1 on
                                          * it, and each time the preset is picked) */
{
    if (bk_preset() != BKP_FLEXOR || (!force && kx2.rsv[52] == FLAY_TAG))
        return;
    arpk_set_fx(4u, OV_RISE);
    kx2.rsv[52] = FLAY_TAG;
}
static void bk_say(void)
{
    char t[24];
    if (ui.bk_sel >= nbk())
        ui.bk_sel = 0u;
    bk_key_name(ui.bk_sel, t);
    if (ui.opage == 2u) {                /* (0.9.49: SEL KEYS, SEL + the key) */
        text_cat(t, " + SEL");
        app_say(t, selb_fn(ui.bk_sel) ? BKF_LONG[selb_fn(ui.bk_sel) - 1u] : "ITS OWN");
        return;
    }
    app_say(t, bk_long(ui.bk_sel));
}
static void bk_pick(uint32_t j)          /* a black key pressed on BLACK KEYS */
{
    ui.bk_sel = (uint8_t)(j % nbk());
    bk_say();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void bk_value(int i, char *t)
{
    t[0] = 0;
    if (ui.opage == 2u) {                /* (0.9.49: SEL KEYS: K2 the key, K3 its SEL function) */
        if (i == 1 && ui.bk_sel == 11u)
            text_cat(t, "REC");
        else if (i == 1)
            int_text((int32_t)ui.bk_sel % 11 + 1, t);
        else if (i == 2)
            text_cat(t, selb_fn(ui.bk_sel) ? BKF_LONG[selb_fn(ui.bk_sel) - 1u] : "ITS OWN");   /* (0.9.70: REC too) */
        return;
    }
    if (i == 0)
        text_cat(t, bk_preset() == BKP_FLEXOR ? "FLEXOR" : BKP_NAME[bk_preset()]);   /* (the foot's room) */
    else if (i == 1 && ui.bk_sel == 11u)
        text_cat(t, "REC");              /* (0.9.58: the REC key) */
    else if (i == 1)
        int_text((int32_t)ui.bk_sel % 11 + 1, t);
    else if (i == 2)
        text_cat(t, ui.bk_sel == 11u && !rec_key_on() ? "NONE" : BKF_LONG[bk_fn(ui.bk_sel)]);   /* (0.9.70: REC records) */
    else if (ui.bk_sel < 11u && bk_fn(ui.bk_sel) == BKF_ARP)  /* (0.9.45: K4, an ARP key's own effect) */
        text_cat(t, OVL_NAME[arpk_fx(ui.bk_sel)]);
}
static void bk_turn(int i, int32_t d)
{
    int dir = d > 0 ? 1 : -1;
    uint32_t j;
    if (set.novice)
        return;
    if (ui.opage == 2u && i != 1) {      /* (0.9.49, FLEXOR: SEL KEYS: K3 the key's function with SEL, ITS OWN (none)
                                          * then every function, round) */
        uint8_t *v = ui.bk_sel == 11u ? &kx2.selrec : &kx2.selk[ui.bk_sel % 11u];   /* (0.9.70, FLEXOR: the REC key's too) */
        if (i != 2)
            return;
        bk_release_all();
        *v = (uint8_t)((selb_fn(ui.bk_sel) + (uint32_t)(dir > 0 ? 1 : BKF_N)) % (BKF_N + 1u));
        if (ui.bk_sel == 11u && *v == 1u + BKF_ARP)   /* (the REC key: any function but ARP, as its own) */
            *v = (uint8_t)((*v + (uint32_t)(dir > 0 ? 1 : BKF_N)) % (BKF_N + 1u));
        bk_say();
        set_dirty = 1u;
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    }
    if (i == 0) {
        uint32_t p = (uint32_t)clamp((int32_t)bk_preset() + dir, 0, BKP_N - 1);
        if (p == bk_preset())
            return;
        bk_release_all();
        set.bkeys.bk[1] = (uint8_t)p;
        flexor_lay_fix(1);               /* (0.9.58: its key 5 RISE) */
        app_say("BLACK KEYS", BKP_NAME[p]);
    } else if (i == 1) {
        ui.bk_sel = (uint8_t)clamp((int32_t)ui.bk_sel + dir, 0, (int32_t)nbk() - 1);   /* (0.9.58: the REC key last) */
        bk_say();
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    } else if (i == 2 && ui.bk_sel == 11u) {   /* (0.9.58: the REC key's own, any function but ARP; 0.9.70, FLEXOR:
                                          * NONE first (REC records), whatever the layout, its preset kept) */
        uint32_t v = rec_key_on() ? rec_fn() + 1u : 0u;
        v = (v + (uint32_t)(dir > 0 ? 1 : BKF_N)) % (BKF_N + 1u);
        if (v == 1u + BKF_ARP)
            v = (v + (uint32_t)(dir > 0 ? 1 : BKF_N)) % (BKF_N + 1u);
        bk_release_all();
        if (v) {
            kx2.rsv[RK_RSV] = (uint8_t)(0x80u | (v - 1u));
            ui.rec = 0u;                 /* (recording no more) */
        }
        kx2.rsv[RK_RSV + 1u] = (uint8_t)(0xA0u | (v ? 1u : 0u));
        (void)j;
        bk_say();
    } else if (i == 2) {
        uint32_t f = (bk_fn(ui.bk_sel) + (uint32_t)(dir > 0 ? 1 : BKF_N - 1)) % BKF_N;
        if (bk_preset() != BKP_CUSTOM) { /* (a preset: its copy, CUSTOM, then the key) */
            for (j = 0; j < 11u; j++)
                set.bkeys.bk[2u + j] = (uint8_t)bk_fn(j);
            set.bkeys.bk[1] = BKP_CUSTOM;
        }
        bk_release_all();
        set.bkeys.bk[2u + ui.bk_sel % 11u] = (uint8_t)f;
        bk_say();
    } else if (ui.bk_sel < 11u && bk_fn(ui.bk_sel) == BKF_ARP) {   /* K4 (0.9.45, FLEXOR): an ARP key's own effect, several ARP keys at
                                          * once each with its own */
        uint32_t e = (arpk_fx(ui.bk_sel) + (dir > 0 ? 1u : 15u)) & 15u;
        arpk_set_fx(ui.bk_sel, e);
        if (arpk_focus() == ui.bk_sel % 11u)
            set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | e);
        ovl_push();
        app_say("ARP FX", OVL_NAME[e]);
    } else
        return;
    set_dirty = 1u;
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* the BLACK KEYS tab: the eleven keys as the FM-1 has them (3, 2, 3, 2, 1), each with its function, the one edited
 * lit; under them that key and its effect, the preset */
static void draw_bk(void)
{
    static const uint8_t GRP[11] = {0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 4};
    uint32_t band, j, p = bk_preset();
    int sl = ui.opage == 2u;             /* (0.9.49: SEL KEYS) */
    char t[40];
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        cv_oy = -(int32_t)band * 52;
        for (j = 0; j < nbk(); j++) {    /* (0.9.58: the REC key last, right, narrower keys to fit) */
            int rk = j == 11u;
            uint32_t f = bk_fn(j), sf = selb_fn(j);   /* (0.9.70: the REC key's SEL function too) */
            int32_t kw = nbk() > 11u ? 15 : 17, x = nbk() > 11u ? (rk ? 212 : 4 + (int32_t)j * 17 + (int32_t)GRP[j] * 4)
                                                                : 4 + (int32_t)j * 19 + (int32_t)GRP[j] * 6;
            int sel = j == ui.bk_sel, chop = rk ? 0 : sl ? !sf : f == BK_LAY[0][j];
            const char *n = sl ? (sf ? BKF_NAME[sf - 1u] : "-") : bk_short(j);
            if (rk)
                kw = 24;
            cv_rect(x, 6, kw, 30, sel ? C_WHITE : chop ? C_LINE : C_AMB);
            cv_rect(x + 1, 7, kw - 2, 28, sel ? C_HI : C_BLOCK);
            t[0] = 0;
            if (rk)
                text_cat(t, "REC");
            else
                int_text((int32_t)j + 1, t);
            cv_text(x + (kw - text_w(&FONT_T, t)) / 2, 9, &FONT_T, t, sel ? C_BLACK : C_DIM);
            cv_text(x + (kw - text_w(&FONT_T, n)) / 2, 23, &FONT_T, n, sel ? C_BLACK : sl && !sf ? C_DIM : C_WHITE);
        }
        bk_key_name(ui.bk_sel, t);
        if (sl) {                        /* (0.9.49, FLEXOR: SEL KEYS: what each key does pressed with SEL) */
            text_cat(t, " WITH SEL");
            cv_text(6, 40, &FONT_XS, t, C_GRAY);
            cv_text(6, 50, &FONT_B, selb_fn(ui.bk_sel) ? BKF_LONG[selb_fn(ui.bk_sel) - 1u] : "ITS OWN", C_HI);
            t[0] = 0;
            text_cat(t, "WITHOUT: ");
            text_cat(t, ui.bk_sel == 11u && !rec_key_on() ? "IT RECORDS" : bk_long(ui.bk_sel));
            cv_text(6, 72, &FONT_XS, t, C_GRAY);
            cv_text(6, 86, &FONT_XS, "SEL + THIS KEY PLAYS THE ONE ABOVE", C_DIM);
            cv_oy = 0;
            cv_blit(0, 88u + band * 52u);
            continue;
        }
        cv_text(6, 40, &FONT_XS, t, C_GRAY);
        if (ui.bk_sel < 11u && bk_fn(ui.bk_sel) == BKF_ARP) {   /* (0.9.45: an ARP key with its own effect, K4) */
            cv_text(6, 50, &FONT_B, "ARP", C_HI);
            cv_text(6 + text_w(&FONT_B, "ARP "), 50, &FONT_B, OVL_NAME[arpk_fx(ui.bk_sel)], C_WHITE);
        } else
            cv_text(6, 50, &FONT_B, bk_long(ui.bk_sel), C_HI);
        cv_text(6, 72, &FONT_XS, "PRESET", C_GRAY);
        cv_text(234 - text_w(&FONT_B, BKP_NAME[p]), 68, &FONT_B, BKP_NAME[p], p == BKP_CUSTOM ? C_AMB : C_WHITE);
        cv_text(6, 86, &FONT_XS, ui.bk_sel == 11u ? "K3: NONE (IT RECORDS) OR A FUNCTION" : p == BKP_CUSTOM
                ? "A BLACK KEY PICKS ITSELF" : "K3: A CUSTOM COPY, THAT KEY CHANGED",
                C_DIM);
        cv_oy = 0;
        cv_blit(0, 88u + band * 52u);
    }
}

static void ovl_value(int i, char *t);
static void ovl_step(int32_t d)          /* (0.9.52, FLEXOR: ALGORITHM, the effect K1-K3 tune: the next or the one before,
                                          * round, nothing played; the ARP key's too, as a key touched here) */
{
    uint32_t e = (ovl_last + (d > 0 ? 1u : 15u)) & 15u;
    char t[16];
    arpk_set_fx(arpk_focus(), e);
    set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | e);
    set_dirty = 1u;
    ovl_push();
    ovl_value(0, t);
    app_say(OVL_NAME[e], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void ovl_value(int i, char *t)
{
    t[0] = 0;
    if (ui.opage >= 1u) {                /* (BLACK KEYS; 0.9.49 SEL KEYS) */
        bk_value(i, t);
        return;
    }
    if (ui.opage) {
        t[0] = 0;
        return;
    }
    if (i == 0) {
        int_text(set.ovl_amt[ovl_last & 15u], t);
        text_cat(t, " %");
    } else if (i == 1)                   /* (0.9.50, FLEXOR: K2 this effect's THRU: BREAK, KICK, THRU; K3 its RATE or
                                          * SWEEP, the one it has) */
        text_cat(t, ARP_THRU[arp_route(ovl_last)]);
    else if (i == 2) {
        int tk = ovl_time_kind(ovl_last);
        text_cat(t, tk == 1 ? OVL_RATE_NAME[set.ovl_rate & 3u] : tk == 2 ? OVL_SWEEP_NAME[set.ovl_sweep & 3u] : "-");
    } else
        text_cat(t, set.ovl_latch ? "ON" : "OFF");
}
static void ovl_turn(int i, int32_t d)
{
    char t[16];
    if (ui.opage >= 1u) {
        bk_turn(i, d);
        return;
    }
    if (ui.opage)
        return;
    if (i == 0) {
        uint8_t *a = &set.ovl_amt[ovl_last & 15u];
        *a = (uint8_t)clamp((int32_t)*a + d * 2, 0, 100);
    } else if (i == 1) {
        arp_thru_turn(ovl_last, d);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    } else if (i == 2 && ovl_time_kind(ovl_last) == 1)
        set.ovl_rate = (uint8_t)clamp((int32_t)set.ovl_rate + (d > 0 ? 1 : -1), 0, 3);
    else if (i == 2 && ovl_time_kind(ovl_last) == 2)
        set.ovl_sweep = (uint8_t)clamp((int32_t)set.ovl_sweep + (d > 0 ? 1 : -1), 0, 3);
    else if (i == 2)
        return;
    else {
        set.ovl_latch = d > 0 ? 1u : 0u;
        if (!set.ovl_latch)
            ovl_latched = 0u;            /* (back to held: what was latched lets go) */
    }
    changed();
    ovl_push();
    ovl_value(i, t);
    app_say(i == 0 ? OVL_NAME[ovl_last & 15u] : i == 2 ? (ovl_time_kind(ovl_last) == 1 ? "RATE" : "SWEEP") : "LATCH", t);
    ui.redraw |= RG_BLOCKS;
}

/* the OVERLAY page's strip: sixteen cards in key order, four by four; one sounding lit, one latched outlined amber,
 * the one K1 tunes outlined white; each with a bar of its amount and its key */
static void draw_ovl(void)
{
    uint32_t band, i, on = ovl_sounding() | ovl_held | ovl_latched | ovl_pad_bits();
    char t[4];
    if (ui.opage >= 1u) {                /* (BLACK KEYS; 0.9.49 SEL KEYS) */
        draw_bk();
        return;
    }
    for (band = 0; band < 2u; band++) {
        cv_begin(240, 52, C_BLACK);
        for (i = band * 8u; i < band * 8u + 8u; i++) {
            int32_t x = 1 + (int32_t)(i % 4u) * 60, y = (int32_t)((i / 4u) & 1u) * 26;
            int lit = on >> i & 1u, lat = ovl_latched >> i & 1u, sel = i == ovl_last;
            cv_rect(x, y, 57, 24, lit ? C_HI : lat ? C_AMB : sel ? C_WHITE : C_LINE);
            cv_rect(x + 1, y + 1, 55, 22, lit ? C_HI : C_BLOCK);
            t[0] = 0;
            int_text((int32_t)i + 1, t);
            cv_text(x + 4, y + 1, &FONT_XS, OVL_NAME[i], lit ? C_BLACK : sel ? C_HI : C_GRAY);
            cv_text(x + 54 - text_w(&FONT_T, t), y + 12, &FONT_T, t, lit ? C_BLACK : C_DIM);   /* its key */
            if (arp_route(i))            /* (0.9.50: its THRU, BREAK or KICK alone) */
                cv_text(x + 4, y + 12, &FONT_T, arp_route(i) == 1u ? "BREAK" : "KICK", lit ? C_BLACK : C_AMB);
            cv_rect(x + 4, y + 18, 34, 2, lit ? C_BLOCK_HI : C_LINE);                         /* its amount */
            cv_rect(x + 4, y + 18, (int32_t)set.ovl_amt[i] * 34 / 100, 2, lit ? C_BLACK : lat ? C_AMB : C_GRAY);
        }
        cv_blit(0, 88u + band * 52u);
    }
}
