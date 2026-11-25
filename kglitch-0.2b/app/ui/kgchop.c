/* SPDX-License-Identifier: MPL-2.0 */
/* K-GLITCH 0.2b: the GLITCH page, SOUNDS' second part (EDIT after the KICK, where the snare, hat and clap were): the
 * CHOP editor. The white keys are K-GLITCH's sixteen CHOP effects (dsp/brk.c KGC_*): on while held, rewriting the
 * break's steps; REC playing, a held effect goes into the CHOP lane, step by step (its automation). K1 the step edited,
 * K2 its effect, K3 the effect's AMOUNT (the held ones', and the lane's steps written from now), K4 the lane's length
 * (OFF 16 32 64). SEL + a white key: that effect out of the lane, at once. The lane and its settings live in kx2 (SAVE).
 * (Part of app.c's unity: included there in order.) */
#define KG_TAG 0x6Bu                     /* (kx2.kg_*: valid) */
static uint32_t kg_held;                 /* the white keys held on the page (bit: KGC_* - 1) */
static uint8_t kg_cur;                   /* the step edited, 0..63 */
static const uint8_t KG_LENS[4] = {0u, 16u, 32u, 64u};
static uint32_t kg_len(void) { return KG_LENS[kx2.kg_len & 3u]; }
static void kg_push(void)                /* the engine: the effect held (the last pressed), the lane */
{
    uint32_t e = 0u, i;
    for (i = 0; i < 16u; i++)
        if (kg_held >> i & 1u)
            e = i + 1u;
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
    memset(kx2.kg_lane, 0, sizeof kx2.kg_lane);
}
static void kg_value(int i, char *t)
{
    uint32_t c = kx2.kg_lane[kg_cur % 64u];
    t[0] = 0;
    if (i == 0)
        int_text(kg_cur + 1, t);
    else if (i == 1)
        text_cat(t, KGC_NAME[(c & 31u) % KGC_N]);
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
        int32_t e = clamp((int32_t)(*c & 31u) + (d > 0 ? 1 : -1), 0, KGC_N - 1);
        *c = (uint8_t)(e ? (uint32_t)e | (uint32_t)(kx2.kg_amt * 7u / 100u) << 5 : 0u);
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
static int kg_keys(uint32_t keys, uint32_t kdn, uint32_t btn)
{
    uint32_t i, was = kg_held;
    if (ui.page != PG_KG) {
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
        if (kdn >> WK[i] & 1u && btn & 1u << B_SEL) {   /* SEL + a key: that effect out of the lane */
            uint32_t k, n = 0u;
            for (k = 0; k < 64u; k++)
                if ((kx2.kg_lane[k] & 31u) == i + 1u)
                    kx2.kg_lane[k] = 0u, n++;
            app_say(KGC_NAME[i + 1u], n ? "OUT OF THE LANE" : "NOT IN THE LANE");
            ui.tap_combo = 1u;
            set_dirty = 1u;
            ui.redraw |= RG_BLOCKS;
            continue;
        }
        if (keys >> WK[i] & 1u && !(btn & 1u << B_SEL))
            kg_held |= 1u << i;
        if (kdn >> WK[i] & 1u && !(btn & 1u << B_SEL))
            app_say(KGC_NAME[i + 1u], ui.rec && playing_now() ? "REC" : "");
    }
    if (kg_held != was) {
        kg_push();
        ui.redraw |= RG_BLOCKS;
    }
    return 1;
}
/* every pass: REC while playing, the effect held goes into the lane on the step playing (the lane on: else 16) */
static void kg_rec(const brk_status_t *st)
{
    static int32_t last = -1;
    uint32_t e = 0u, i, len;
    if (!kg_held || !ui.rec || !playing_now() || st->step < 0) {
        last = -1;
        return;
    }
    if (!kg_len())
        kx2.kg_len = 1u;
    len = kg_len();
    for (i = 0; i < 16u; i++)
        if (kg_held >> i & 1u)
            e = i + 1u;
    if (st->step == last)
        return;
    last = st->step;
    kx2.kg_lane[(uint32_t)st->step % len] = (uint8_t)(e | (uint32_t)(kx2.kg_amt * 7u / 100u) << 5);
    kg_cur = (uint8_t)((uint32_t)st->step % len);
    set_dirty = 1u;
    kg_push();
    ui.redraw |= RG_BLOCKS;
}
/* the four as cards, then the lane's 16 steps of the page of the step edited (the playhead white), the effect playing */
static void draw_kg(const brk_status_t *st)
{
    uint32_t i, len = kg_len() ? kg_len() : 16u, pg = (kg_cur % len) / 16u, now = (uint32_t)brk_kg_now();
    static const char *const N[4] = {"STEP", "EFFECT", "AMOUNT", "LANE"};
    char t[16];
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        cv_rect(x, 0, 56, 50, C_BLOCK);
        cv_text(x + 4, 3, &FONT_XS, N[i], C_GRAY);
        kg_value((int)i, t);
        if (i == 1u) {
            t[0] = 0;
            text_cat(t, KGC_SHORT[(kx2.kg_lane[kg_cur % 64u] & 31u) % KGC_N]);
        }
        cv_text(x + 4, 30, &FONT_T, t, i == 3u && !kg_len() ? C_DIM : C_HI);
    }
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < 16u; i++) {
        uint32_t p = pg * 16u + i, c = kx2.kg_lane[p % 64u] & 31u;
        int32_t x = 2 + (int32_t)i * 15;
        int on = st->step >= 0 && kg_len() && (uint32_t)st->step % len == p;
        cv_rect(x, 2, 13, 22, p == kg_cur ? C_WHITE : c ? C_AMB : (i & 3u) ? C_BLOCK : C_DIM);
        if (c && x)
            cv_text(x + 1, 6, &FONT_XS, KGC_SHORT[c % KGC_N], p == kg_cur ? C_BLACK : C_BLACK);
        if (on)
            cv_rect(x, 25, 13, 3, C_WHITE);
    }
    t[0] = 0;
    text_cat(t, now ? KGC_NAME[now % KGC_N] : kg_len() ? "LANE ON" : "KEYS: THE 16 CHOP FX");
    cv_text(6, 32, &FONT_XS, t, now ? C_WHITE : C_DIM);
    cv_text(130, 32, &FONT_XS, ui.rec ? "REC: HELD FX WRITTEN" : "SEL+KEY: FX OUT", ui.rec ? C_AMB : C_DIM);
    cv_blit(0, 140);
}
static void kg_boot(void)                /* (app.c, at power-on: a save without the lane: its defaults) */
{
    if (kx2.kg_tag != KG_TAG)
        kg_defaults();
    kg_push();
}
