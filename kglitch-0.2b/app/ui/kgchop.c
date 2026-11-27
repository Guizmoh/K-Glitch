/* SPDX-License-Identifier: MPL-2.0 */
/* K-GLITCH 0.2b: the GLITCH page, SOUNDS' second part (EDIT after the KICK, where the snare, hat and clap were): the
 * CHOP editor. The white keys are K-GLITCH's sixteen CHOP effects (dsp/brk.c KGC_*): on while held, rewriting the
 * break's steps; REC playing, a held effect goes into the CHOP lane, step by step (its automation). K1 the step edited,
 * K2 its effect, K3 the effect's AMOUNT (the held ones', and the lane's steps written from now), K4 the lane's length
 * (OFF 16 32 64). SEL + a white key: that effect out of the lane, at once. The lane and its settings live in kx2 (SAVE).
 * (Part of app.c's unity: included there in order.) */
#define KG_TAG 0x6Cu                     /* (0.2b: the lane's bytes bits 0-5 the effect (48), 6-7 its amount) */                     /* (kx2.kg_*: valid) */
static uint32_t kg_held;
static uint32_t WK_MASK;                 /* (the white keys' bits, kg_boot) */                 /* the white keys held on the page (bit: KGC_* - 1) */
static uint8_t kg_cur;                   /* the step edited, 0..63 */
static const uint8_t KG_LENS[4] = {0u, 16u, 32u, 64u};
static uint32_t kg_len(void) { return KG_LENS[kx2.kg_len & 3u]; }
#define kg_bank ((kx2.kg_rsv & 3u) % KGC_BANKS)   /* CHOP, DESTROY, BEAT: the white keys' sixteen (OCT- / OCT+ on the page) */
static uint8_t kg_lb(uint32_t e) { return (uint8_t)(e ? e | (kx2.kg_amt * 3u / 100u) << 6 : 0u); }   /* a lane byte */
static void kg_push(void)                /* the engine: the effect held (the last pressed), the lane */
{
    uint32_t e = 0u, i;
    for (i = 0; i < 16u; i++)
        if (kg_held >> i & 1u)
            e = kg_bank * 16u + i + 1u;
    if (kx2.kg_tag != KG_TAG)
        brk_kg(0u, 0, 0u);
    else
        brk_kg(e | (uint32_t)kx2.kg_amt << 8, kx2.kg_lane, kg_len());
}
static void kg_defaults(void)
{
    kx2.kg_tag = KG_TAG;
    kx2.kg_len = 1u;
    kx2.kg_amt = 60u;
    kx2.kg_rsv = 0u;
    memset(kx2.kg_lane, 0, sizeof kx2.kg_lane);
}
static void kg_value(int i, char *t)
{
    uint32_t c = kx2.kg_lane[kg_cur % 64u];
    t[0] = 0;
    if (i == 0)
        int_text(kg_cur + 1, t);
    else if (i == 1)
        text_cat(t, KGC_NAME[(c & 63u) % KGC_N]);
    else if (i == 2) {
        int_text(kx2.kg_amt, t);
        text_cat(t, "%");
    } else if (!kg_len())
        text_cat(t, "OFF");
    else
        int_text((int32_t)kg_len(), t);
}
static void kg_turn(int i, int32_t d)
{
    static const char *const N[4] = {"STEP", "EFFECT", "AMOUNT", "LANE"};
    char t[16];
    uint8_t *c = &kx2.kg_lane[kg_cur % 64u];
    if (kx2.kg_tag != KG_TAG)
        kg_defaults();
    if (i == 0)
        kg_cur = (uint8_t)clamp((int32_t)kg_cur + d, 0, (int32_t)(kg_len() ? kg_len() : 16u) - 1);
    else if (i == 1) {
        int32_t e = clamp((int32_t)(*c & 63u) + (d > 0 ? 1 : -1), 0, KGC_N - 1);
        *c = kg_lb((uint32_t)e);
    } else if (i == 2)
        kx2.kg_amt = (uint8_t)clamp((int32_t)kx2.kg_amt + d * 2, 0, 100);
    else
        kx2.kg_len = (uint8_t)clamp((int32_t)(kx2.kg_len & 3u) + (d > 0 ? 1 : -1), 0, 3);
    kg_push();
    set_dirty = 1u;
    kg_value(i, t);
    app_say(N[i & 3], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* every input: on the page the white keys are the effects (SEL + a key: its effect out of the lane); off the page,
 * whatever was held lets go */
/* K-GLITCH 0.2b: on HOME (PERFORM, GENERATOR, SPLIT, CHAOS) OCT- and OCT+ are SHIFTs for the white keys: OCT- held, a
 * key holds its CHOP effect (as on the GLITCH page; REC playing writes it into the lane); OCT+ held, a key holds its ARP
 * effect (the OVERLAY's sixteen). OCT tapped alone: the break's pitch, as before (at its release, main.c) */
static uint8_t oct_used;                 /* an OCT held on HOME played a key: its release no pitch */
static uint32_t oct_shift(uint32_t btn)  /* 1 OCT- alone held on HOME, 2 OCT+ alone, 0 none */
{
    uint32_t o = btn & (1u << B_OCTDN | 1u << B_OCTUP);
    if (!HOME_PG(ui.page) || ui.page == PG_KG || (ui.rec && !playing_now()))
        return 0u;
    return o == 1u << B_OCTDN ? 1u : o == 1u << B_OCTUP ? 2u : 0u;
}
static int kg_keys(uint32_t keys, uint32_t kdn, uint32_t btn)
{
    uint32_t i, was = kg_held, sh = oct_shift(btn), ov = 0u;
    if (sh == 2u)                        /* OCT+: the ARP effects, while the keys are held */
        for (i = 0; i < 16u; i++) {
            if (keys >> WK[i] & 1u)
                ov |= 1u << i;
            if (kdn >> WK[i] & 1u) {
                oct_used = 1u;
                app_say(OVL_NAME[i], "OCT+ ARP");
            }
        }
    if (ov != oct_ovl) {
        oct_ovl = ov;
        ovl_push();
        ui.redraw |= RG_BLOCKS;
    }
    if (sh == 2u) {
        if (kg_held) {
            kg_held = 0u;
            kg_push();
        }
        return 1;
    }
    if (sh == 1u && kdn & WK_MASK)
        oct_used = 1u;
    if (ui.page != PG_KG && sh != 1u) {
        if (kg_held) {
            kg_held = 0u;
            kg_push();
        }
        return 0;
    }
    if (kx2.kg_tag != KG_TAG)
        kg_defaults();
    kg_held = 0u;
    for (i = 0; i < 16u; i++) {
        if (kdn >> WK[i] & 1u && btn & 1u << B_SEL && ui.page == PG_KG) {   /* SEL + a key: that effect out of the lane */
            uint32_t k, n = 0u;
            for (k = 0; k < 64u; k++)
                if ((kx2.kg_lane[k] & 63u) == kg_bank * 16u + i + 1u)
                    kx2.kg_lane[k] = 0u, n++;
            app_say(KGC_NAME[kg_bank * 16u + i + 1u], n ? "OUT OF THE LANE" : "NOT IN THE LANE");
            ui.tap_combo = 1u;
            set_dirty = 1u;
            ui.redraw |= RG_BLOCKS;
            continue;
        }
        if (keys >> WK[i] & 1u && !(btn & 1u << B_SEL))
            kg_held |= 1u << i;
        if (kdn >> WK[i] & 1u && !(btn & 1u << B_SEL))
            app_say(KGC_NAME[kg_bank * 16u + i + 1u], ui.rec && playing_now() ? "REC" : sh == 1u ? "OCT- SHIFT" : "");
    }
    if (kg_held != was) {
        kg_push();
        ui.redraw |= RG_BLOCKS;
    }
    return 1;
}
/* every pass: REC while playing, the effect held goes into the lane on the step playing (the lane on: else 16) */
static void kg_bank_turn(int32_t d)      /* OCT- / OCT+ on the GLITCH page: the white keys' bank */
{
    if (kx2.kg_tag != KG_TAG)
        kg_defaults();
    kx2.kg_rsv = (uint8_t)((kx2.kg_rsv & 0xC0u) | (kg_bank + KGC_BANKS + (d > 0 ? 1u : KGC_BANKS - 1u)) % KGC_BANKS);
    set_dirty = 1u;
    app_say("BANK", KGC_BANK_NAME[kg_bank]);
    kg_push();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void kg_rec(const brk_status_t *st)
{
    static int32_t last = -1;
    uint32_t e = 0u, i, len;
    {                                    /* (the DESTROY bank's sounds: the ARP's RING, DIST, RADIO while they play) */
        int32_t now = brk_kg_now();
        uint32_t m = now == KGD_RING ? 1u << OV_RING : now == KGD_FUZZ ? 1u << OV_DIST : now == KGD_FILTER ? 1u << OV_RADIO : 0u;
        if (!st->playing)
            m = 0u;
        if (m != kg_ovl) {
            kg_ovl = m;
            ovl_push();
        }
    }
    if (!kg_held || !ui.rec || !playing_now() || st->step < 0) {
        last = -1;
        return;
    }
    if (!kg_len())
        kx2.kg_len = 1u;
    len = kg_len();
    for (i = 0; i < 16u; i++)
        if (kg_held >> i & 1u)
            e = kg_bank * 16u + i + 1u;
    if (st->step == last)
        return;
    last = st->step;
    kx2.kg_lane[(uint32_t)st->step % len] = kg_lb(e);
    kg_cur = (uint8_t)((uint32_t)st->step % len);
    set_dirty = 1u;
    kg_push();
    ui.redraw |= RG_BLOCKS;
}
/* RIBBON CLASSIC (K-GLITCH 0.2b): a card, its title, the bank as an orange pill, an orange rule, the lane and the step;
 * then the lane's 16 steps of the page edited as bars (DASH: the bar's height its amount, its colour its bank: CHOP
 * orange, DESTROY cyan, BEAT yellow; its effect's letter above; the step edited outlined, the playhead under) */
static void draw_kg(const brk_status_t *st)
{
    uint32_t i, len = kg_len() ? kg_len() : 16u, pg = (kg_cur % len) / 16u, now = (uint32_t)brk_kg_now();
    char t[40];
    int32_t w;
    cv_begin(240, 52, C_BLACK);
    rbox(3, 0, 234, 52, C_BLOCK);
    cv_text(9, 2, &FONT_B, "GLITCH", C_WHITE);
    w = text_w(&FONT_T, KGC_BANK_NAME[kg_bank]) + 16;
    rbox(230 - w, 3, w, 14, C_AMB);
    cv_text(230 - w + 8, 4, &FONT_T, KGC_BANK_NAME[kg_bank], C_BLACK);
    {                                    /* (the lane and the step, small, beside the title) */
        char n[8];
        t[0] = 0;
        text_cat(t, "L");
        if (kg_len()) int_text((int32_t)kg_len(), n); else n[0] = '-', n[1] = 0;
        text_cat(t, n);
        text_cat(t, " S");
        int_text(kg_cur + 1, n);
        text_cat(t, n);
        if (ui.rec) text_cat(t, " REC");
        cv_text(80, 6, &FONT_T, t, ui.rec ? C_AMB : C_GRAY);
    }
    cv_rect(9, 20, 222, 1, C_AMB);
    for (i = 0; i < 16u; i++) {          /* the white keys and their effects (this bank): held orange, playing outlined */
        uint32_t e = kg_bank * 16u + i + 1u;
        int32_t x = 8 + (int32_t)(i % 8u) * 28, y = 23 + (int32_t)(i / 8u) * 14;
        int held = kg_held >> i & 1u;
        if (now == e)
            rbox(x - 1, y - 1, 28, 14, C_WHITE);
        rbox(x, y, 26, 12, held ? C_AMB : C_LINE);
        cv_text(x + (26 - text_w(&FONT_T, KGC_SHORT[e % KGC_N])) / 2, y, &FONT_T, KGC_SHORT[e % KGC_N], held ? C_BLACK : C_WHITE);
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    rbox(3, 0, 234, 52, C_BLOCK);
    for (i = 0; i < 16u; i++) {
        uint32_t p = pg * 16u + i, b = kx2.kg_lane[p % 64u], e = b & 63u, lv = b >> 6;
        int32_t x = 8 + (int32_t)i * 14, h = e ? 14 + (int32_t)(lv ? lv : 2u) * 9 : 3;
        uint16_t col = !e ? C_LINE : e <= 16u ? C_AMB : e <= 32u ? C_HI : RGB(255, 196, 48);
        int on = st->step >= 0 && kg_len() && (uint32_t)st->step % len == p;
        if (p == kg_cur)
            rbox(x - 1, 46 - h - 1, 13, h + 2, C_WHITE);
        rbox(x, 46 - h, 11, h, col);
        if (e) {
            char l[2] = {KGC_SHORT[e % KGC_N][0], 0};
            cv_text(x + 3, 46 - h - 11 < 0 ? 0 : 46 - h - 11, &FONT_T, l, C_WHITE);
        }
        if (on)
            cv_rect(x, 48, 11, 3, C_WHITE);
        else if (!(i & 3u))
            cv_rect(x, 49, 11, 1, C_DIM);
    }
    cv_blit(0, 140);
}
static void kg_boot(void)                /* (app.c, at power-on: a save without the lane: its defaults) */
{
    uint32_t i;
    for (i = 0, WK_MASK = 0u; i < 16u; i++)
        WK_MASK |= 1u << WK[i];
    if (kx2.kg_tag != KG_TAG)
        kg_defaults();
    kg_push();
}
