/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the black-key pads: their own settings (K1-K4 while held) and their animations. (Part of app.c's unity:
 * included there in order.) */

/* -------------------------------------------------- the pads' own settings --- */
static void ovl_push(void);              /* (ovl.c) */
static const char *const THRU_NAME[4] = {"BOTH", "BREAK", "KICK", "BOTH"};   /* VINYL's THRU */
static const char *const ARP_THRU[4] = {"THRU", "BREAK", "KICK", "THRU"};   /* (0.9.29, FLEXOR: what the ARP's effects take,
                                          * ovl_pad 6-7: 0 the whole mix, 1 the break, 2 the kick; also LFO's LEVELS K4, SENDS K3) */
static void arp_thru_turn(uint32_t e, int32_t d)   /* BREAK, KICK, THRU in that order (FLEXOR); 0.9.50, FLEXOR: each
                                          * effect its own (app.c arp_route), saved */
{
    static const uint8_t AT[3] = {1u, 2u, 0u}, POS[3] = {2u, 0u, 1u};
    uint32_t c = AT[clamp((int32_t)POS[arp_route(e)] + (d > 0 ? 1 : -1), 0, 2)];
    arp_route_set(e, c);
    set_dirty = 1u;
    app_say(OVL_NAME[e & 15u], ARP_THRU[c]);
}
/* LOAF BREAKER (0.9.45, FLEXOR): every black key set to ARP is its own: its effect and its LATCH, several sounding at
 * once (THRU, ON / OFF, RATE, SWEEP and each effect's amount shared). Kept in rsv2 (v0.52's fills' bytes), tagged
 * ARPK_TAG in rsv2[0]: rsv2[1..6] an effect a nibble, key j's at 1 + j / 2 (even j the low nibble), the LATCH bits of
 * keys 0-7 in rsv2[7], of keys 8-10 in rsv2[6]'s bits 4-6. Untagged (an older save, a reset): every key ovl_pad's
 * (bits 0-3 the effect, bit 4 LATCH), as when there was one ARP for all */
#define ARPK_TAG 0xA7u
static uint16_t arp_held, arp_lat;       /* (bit j: ARP key j held; switched on, LATCH) */
static uint8_t arp_foc;                  /* the ARP key touched last: its knobs, the OVERLAY page's touches */
static uint32_t arpk_fx(uint32_t j)
{
    j %= 11u;
    if (set.rsv2[0] != ARPK_TAG)
        return set.ovl_pad & 15u;
    return (uint32_t)(set.rsv2[1u + j / 2u] >> (j & 1u ? 4 : 0)) & 15u;
}
static uint32_t arpk_latch(uint32_t j)
{
    j %= 11u;
    if (set.rsv2[0] != ARPK_TAG)
        return (uint32_t)(set.ovl_pad >> 4) & 1u;
    return j < 8u ? (uint32_t)(set.rsv2[7] >> j) & 1u : (uint32_t)(set.rsv2[6] >> (j - 4u)) & 1u;
}
static void arpk_own(void)               /* (untagged: each key takes ovl_pad's, then its own) */
{
    uint32_t e = set.ovl_pad & 15u, l = (uint32_t)(set.ovl_pad >> 4) & 1u, i;
    if (set.rsv2[0] == ARPK_TAG)
        return;
    set.rsv2[0] = ARPK_TAG;
    for (i = 1u; i <= 5u; i++)
        set.rsv2[i] = (uint8_t)(e | e << 4);
    set.rsv2[6] = (uint8_t)(e | (l ? 0x70u : 0u));
    set.rsv2[7] = l ? 0xFFu : 0u;
}
static void arpk_set_fx(uint32_t j, uint32_t e)
{
    uint8_t *b;
    j %= 11u;
    arpk_own();
    b = &set.rsv2[1u + j / 2u];
    *b = (uint8_t)(j & 1u ? (*b & 0x0Fu) | (e & 15u) << 4 : (*b & 0xF0u) | (e & 15u));
}
static void arpk_set_latch(uint32_t j, int on)
{
    uint8_t *b, m;
    j %= 11u;
    arpk_own();
    b = j < 8u ? &set.rsv2[7] : &set.rsv2[6];
    m = (uint8_t)(1u << (j < 8u ? j : j - 4u));
    *b = (uint8_t)(on ? *b | m : *b & ~m);
}
static int arpk_on(uint32_t j)           /* (ARP key j sounding its effect) */
{
    j %= 11u;
    return ((arp_held | arp_lat) >> j & 1u) && !(set.ovl_pad & 32u) && bk_fn(j) == BKF_ARP;   /* (bit 5: K3 OFF) */
}
static uint32_t arpk_count(void)         /* (the ARP keys in the layout) */
{
    uint32_t j, n = 0u;
    for (j = 0; j < 11u; j++)
        n += bk_fn(j) == BKF_ARP;
    return n;
}
static uint32_t arpk_focus(void)         /* (the ARP key touched last; none yet, or no longer ARP: the layout's first) */
{
    uint32_t j;
    if (bk_fn(arp_foc % 11u) != BKF_ARP)
        for (j = 0; j < 11u; j++)
            if (bk_fn(j) == BKF_ARP) {
                arp_foc = (uint8_t)j;
                break;
            }
    return arp_foc % 11u;
}
static int ovl_pad_on(void);
/* LOAF BREAKER: the three roll pads' THRU (MIX › SENDS K4 since 0.9.40, FLEXOR: their K2 the break's filter, to roll
 * and filter at once; K2 0.245-0.9.39; set.pads.rsv[0]): 0 BREAK (the roll as ever), 1 KICK (the kick rolled
 * at the pad's rate, the break left alone), 2 BOTH. kroll: the pad rolling the kick (app.c's render) */
static const char *const ROLL_THRU_NAME[3] = {"BREAK", "KICK", "BOTH"};
static uint8_t roll_held = 0xFFu;        /* the roll pad held (0xFF none) */
static uint8_t roll_sel;                 /* (pressed with SEL held: the kick alone, whatever THRU says) */
static void roll_thru_now(void)          /* (THRU turned while a roll pad is held: at once) */
{
    uint32_t th = set.pads.rsv[0] % 3u;
    if (roll_held > PAD_ROLL32 || roll_sel)
        return;
    brk_pad((int)roll_held, th != 1u);
    kroll = (uint8_t)(th ? roll_held + 1u : 0u);
}
static void roll_thru_turn(int32_t d)    /* (SENDS' K4: BREAK, KICK, BOTH) */
{
    set.pads.rsv[0] = (uint8_t)clamp((int32_t)(set.pads.rsv[0] % 3u) + (d > 0 ? 1 : -1), 0, 2);
    roll_thru_now();
    app_say("ROLL", ROLL_THRU_NAME[set.pads.rsv[0] % 3u]);
}
static void roll_key(uint32_t pad, int down, int sel)   /* a roll pad down or up, through its THRU (SEL held as it
                                          * went down: the kick alone, for that press) */
{
    uint32_t th = sel ? 1u : set.pads.rsv[0] % 3u;
    if (down) {
        roll_held = (uint8_t)pad;
        roll_sel = (uint8_t)(sel != 0);
        if (th != 1u)
            brk_pad((int)pad, 1);
        kroll = (uint8_t)(th ? pad + 1u : 0u);
    } else {
        brk_pad((int)pad, 0);
        if (roll_held == pad) {
            roll_held = 0xFFu;
            roll_sel = 0u;
            kroll = 0u;
        }
    }
}
static int roll_lit(uint32_t pad) { return roll_held == pad && kroll; }   /* (its light, rolling the kick alone) */
/* (0.9.49) black key j's function as the lights and the strip show it: SEL held or locked, its SEL KEYS function
 * where it has one */
static uint32_t bk_fn_shown(uint32_t j)
{
    return (ui.sel_raw || ui.sel_lock) && selb_fn(j) ? selb_fn(j) - 1u : bk_fn(j);
}
static uint8_t half_lat;                 /* LOAF BREAKER: HALF SPEED latched on (its LATCH: K3) */
static uint16_t pad_latched;             /* (0.9.26: REVERSE, FREEZE, GATE, RANDOM latched on, bit by pad) */
static const char *const FTHRU_NAME[3] = {"BREAK", "DRUMS", "BOTH"};
/* what K1-K4 tune while a pad is held, by pad (brk.h PAD_*); 0: that knob does nothing for it */
static const char *const PAD_KNOB[BKF_N][4] = {   /* (LOAF BREAKER: by function, app.c BKF_*) */
    {"RATE", "BRK FLT", "DRUM FLT", "DECAY"}, {"RATE", "BRK FLT", "DRUM FLT", "DECAY"}, {"RATE", "BRK FLT", "DRUM FLT", "DECAY"},
    {"SPEED", 0, 0, "LATCH"}, {"LENGTH", "PITCH", 0, "LATCH"}, {"SPEED", "M. TEMPO", "LATCH", 0},
    {"THRU", "LATCH", "STOP", "SPIN UP"}, {"RATE", "LENGTH", 0, "LATCH"}, {"AMOUNT", "PITCH", 0, "LATCH"},
    {"TARGET", "RESO", "SPEED", "THRU"},   /* (0.9.26, FLEXOR: K4 LATCH on the keys with a knob free; FILTER's K4
                                          * THRU: BREAK, DRUMS, BOTH) */
    {"TIME", "FEEDBACK", "TONE", "MIX"},   /* (DUB: CHOPPAH's; a DELAY in the chain: its own, fx_param_name) */
    {"EFFECT", "AMOUNT", "TIME", "LATCH"},   /* (the ARP key's; FX while held: MORE, pad_knob_name; K2 what goes
                                          * through the ARP effects: the kick, the break, or both) */
    {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {"MODE", "TIME", "REVERB", "LATCH"},   /* (0.9.58: KILL BREAK) */
    {0, 0, 0, 0}};                       /* (0.9.76: CHS, a press each) */   /* (KIL, X2: their LATCH, main.c; DST,
                                          * SUB, DRV: a press each) */
static const char *const PAD_TITLE[BKF_N] = {"ROLL", "ROLL", "ROLL", "REVERSE", "FREEZE", "HALF SPEED",
                                              "VINYL BREAK", "GATE", "RANDOM", "FILTER", "DUB", "OVERLAY", "KICKS OFF",
                                              "DISTO", "SUB FX", "DRIVE", "KICKS X2", "KICKS X4", "KICK TRIPLETS",
                                              "SUB DRIVE", "KILL BREAK", "CHAOS"};
static const char *const FRZ_LEN_NAME[4] = {"SLICE", "1/2", "1/4", "1/8"};

/* LOAF BREAKER: the ARP key's knobs (the OVERLAY page gone, its settings here). MAIN: K1 EFFECT, K2 its AMOUNT, K3
 * its TIME (RATE for CHOP, GATE, WAH, PAN; SWEEP for DIVE and RISE; none for the rest), K4 LATCH. MORE (FX pressed while
 * the key is held): K1 THRU (what goes through: the kick, the break, both), K2 ON / OFF (OFF: K1 browses silent), K3
 * RATE, K4 SWEEP. ui.pad_more */
static int ovl_time_kind(uint32_t e)     /* 1 RATE, 2 SWEEP, 0 none */
{
    return e == OV_CHOP || e == OV_GATE || e == OV_PAN ? 1 : e == OV_DIVE || e == OV_RISE ? 2 : 0;
}
/* LOAF BREAKER (FLEXOR, 11 Nov 2026): BEGINNER's pads have their knobs as EXPERT's (0.232; 0.224-0.231 none but the
 * ARP key's K1, K2); the DST and SUB keys do nothing there (main.c) */
static const char *pad_knob_name(int pad, int k)
{
    if (pad < 0 || pad >= (int)BKF_N)
        return 0;
    if (pad == PAD_DUB && delay_slot() >= 0)   /* (a DELAY in the chain: the pad throws into it, its settings) */
        return fx_param_name((uint32_t)delay_slot(), k)[0] ? fx_param_name((uint32_t)delay_slot(), k) : 0;
    if (pad == BKF_ARP) {
        static const char *const MORE[4] = {"THRU", "ON / OFF", "RATE", "SWEEP"};
        if (ui.pad_more)
            return MORE[k & 3];
        if (k == 2) {
            int tk = ovl_time_kind(arpk_fx(arpk_focus()));
            return tk == 1 ? "RATE" : tk == 2 ? "SWEEP" : "TIME";
        }
    }
    if (pad == BKF_KBR && k == 1 && kb_mode() == KB_BAR)
        return "ECHO";                   /* (0.9.60: BAR END's K2, its echo) */
    return PAD_KNOB[pad][k & 3];
}
/* -1..1 as the DJ filter's frequency: "LP 340HZ" / "HP 1.2K" */
static void dj_text(float dj, char *t)
{
    t[0] = 0;
    text_cat(t, dj > 0.0f ? "HP " : "LP ");
    hz_text(dj > 0.0f ? 20.0f * brk_exp2f(dj * 8.2f) : 18000.0f * brk_exp2f(dj * 7.8f), t + 3);   /* (common.c's) */
}

/* LOAF BREAKER: HOME's two filters, K2 the break's and K3 the drums' (SEQ's K2 and the roll pads' the same), work
 * alike: from OFF at the centre, left a low-pass, right a high-pass; away from OFF 2 % a detent, the first already heard; back towards OFF 4 % a detent (out faster
 * than in), OFF once past the first. Back at OFF it holds: turning on the same way stays OFF until the knob rests half a
 * second (no sliding into the other kind); the other way, at once. u: + right of OFF, - left; s_right, s_left: the
 * first detent's place; lk: that knob's hold */
typedef struct { uint32_t t; int8_t dir; } flt_lock_t;
static flt_lock_t lock_brk, lock_drum;
static float flt_turn(float u, int32_t d, float s_right, float s_left, flt_lock_t *lk)
{
    uint32_t now = plat_ms();
    if (u > -0.001f && u < 0.001f) {
        if (lk->dir && (d > 0) == (lk->dir > 0) && now - lk->t < 500u) {   /* still turning on past OFF: held */
            lk->t = now;
            return 0.0f;
        }
        lk->dir = 0;
        u = d > 0 ? s_right + 0.02f * (float)(d - 1) : -(s_left + 0.02f * (float)(-d - 1));
    } else if (u > 0.0f) {
        u += (d > 0 ? 0.02f : 0.04f) * (float)d;
        if (u < s_right - 0.001f) {
            u = 0.0f;
            lk->dir = -1;
            lk->t = now;
        }
    } else {
        u += (d < 0 ? 0.02f : 0.04f) * (float)d;
        if (u > -(s_left - 0.001f)) {
            u = 0.0f;
            lk->dir = 1;
            lk->t = now;
        }
    }
    return brk_clampf(u, -1.0f, 1.0f);
}

/* the drums' (set.kick_dj, -100 low-pass .. 100 high-pass): right the high-pass from 55 Hz, left the low-pass from
 * 7.6 kHz */
static void kick_flt_text(char *t)
{
    t[0] = 0;
    text_cat(t, set.kick_dj > 0 ? "HP " : set.kick_dj < 0 ? "LP " : "OFF");
    if (set.kick_dj)
        hz_text(brk_fx_drum_hz((float)set.kick_dj * 0.01f, dft_kind()), t + 3);   /* (0.9.67: its kind's) */
}

static uint8_t kdj_quiet;                /* (0.9.64: the drums' filter turned while KIL had the kicks quiet) */
static void kick_flt_turn(int32_t d)
{
    char t[16];
    if (kick_hold)
        kdj_quiet = 1u;
    if (dft_kind() == BRK_DFT_SMOOTH)
        d *= 2;                          /* (0.9.68, FLEXOR: SMOOTH not reactive enough, a detent two) */
    float v = flt_turn((float)set.kick_dj * 0.01f, d, 0.02f, 0.16f, &lock_drum);
    set.kick_dj = (int8_t)(int32_t)(v * 100.0f + (v < 0.0f ? -0.5f : 0.5f));
    changed();
    kick_flt_text(t);
    app_say("DRUM FILTER", t);
}

/* (0.9.50, FLEXOR: the kicks back on with KIL, the drums' filter OFF at once: never a drop filtered by mistake;
 * 0.9.64, FLEXOR: SYSTEM › EFFECTS K4 KICK SWITCH: SMART keeps it when it was turned while the kicks were quiet, the
 * kicks brought back into it; FLT OFF as before; FLT KEEP never) */
static void kil_back_flt(void)
{
    uint32_t m = ksw_mode(), q = kdj_quiet;
    kdj_quiet = 0u;
    if (!set.kick_dj || m == KSW_KEEP || (m == KSW_SMART && q))
        return;
    set.kick_dj = 0;
    lock_drum.dir = 0;
    changed();
}

/* the break's (set.fx.dj, -1 low-pass .. 1 high-pass): left the low-pass from 7.6 kHz, right the high-pass from
 * 110 Hz (as the drums': low-pass left, high-pass right) */
static void brk_flt_text(char *t)
{
    if (set.fx.dj < -0.01f || set.fx.dj > 0.01f)
        dj_text(set.fx.dj, t);
    else {
        t[0] = 0;
        text_cat(t, "OFF");
    }
}

static void brk_flt_turn(int32_t d)
{
    char t[16];
    set.fx.dj = flt_turn(set.fx.dj, d, 0.3f, 0.16f, &lock_brk);   /* (left the low-pass, right the high-pass) */
    changed();
    brk_flt_text(t);
    app_say("DJ FILTER", t);
}

/* the roll's decay: "DOWN 20%" each repeat quieter, "UP 20%" a swell, "OFF" */
static void roll_decay_text(float dc, char *t)
{
    char v[8];
    t[0] = 0;
    if (dc > -0.01f && dc < 0.01f) {
        text_cat(t, "OFF");
        return;
    }
    text_cat(t, dc > 0.0f ? "DOWN " : "UP ");
    pct_text(dc > 0.0f ? dc : -dc, v);
    text_cat(t, v);
}

static void pad_knob_value(int pad, int k, char *t)
{
    const brk_padset_t *p = &set.pads;
    const brk_fx_t *f = &set.fx;
    t[0] = 0;
    switch (pad <= PAD_ROLL32 ? 0 : pad) {
    case 0:
        if (k == 0) text_cat(t, BRK_ROLL_DIV_NAME[p->roll_div[pad] % BRK_ROLL_DIVS]);
        else if (k == 1) brk_flt_text(t);   /* LOAF BREAKER: K2 HOME's break filter (0.9.40, FLEXOR; THRU before:
                                          * SENDS' K4), K3 HOME's drums' filter, DECAY on K4 (the pitch and LEVEL gone) */
        else if (k == 2) kick_flt_text(t);
        else roll_decay_text(p->roll_decay, t);
        break;
    case BKF_KBR: {                      /* (0.9.58: KILL BREAK: MODE, its fade's TIME, BAR END's REVERB, LATCH) */
        static const char *const KM[KB_N] = {"FADE", "CUT", "BAR END", "END FADE"}, *const KT[KB_TIMES] = {"1/16", "1/8", "1/4",
                                                                                               "1/2", "1 BAR", "2 BARS"};
        if (k == 0)
            text_cat(t, KM[kb_mode()]);
        else if (k == 1 && kb_mode() == KB_BAR) {   /* (0.9.60: ECHO, its time) */
            static const char *const KE[KB_ECHOS] = {"OFF", "1/8", "3/16", "1/4", "3/8"};
            text_cat(t, KE[kb_echo()]);
        } else if (k == 1)
            text_cat(t, kb_mode() == KB_FADE || kb_mode() == KB_ENDF ? KT[kb_time()] : "-");
        else if (k == 2) {
            if (kb_mode() != KB_BAR)
                text_cat(t, "-");
            else
                pct_text((float)kb_rev() * 0.01f, t);
        } else
            text_cat(t, kb_latch() ? "ON" : "OFF");
        break;
    }
    case PAD_REVERSE: if (k == 3) goto lat; times_text(p->rev_speed, t); break;
    case PAD_HALF:                       /* (GLOUPI BREAKS: K2 its master tempo) */
        if (k == 0) times_text(p->half_speed, t);
        else if (k == 1) text_cat(t, set.half_mt_off & 1u ? "OFF" : "ON");
        else text_cat(t, set.half_mt_off & 2u ? "ON" : "OFF");   /* (LOAF BREAKER: K3 LATCH) */
        break;
    case PAD_TAPESTOP:                   /* (LOAF BREAKER: the VINYL BREAK; 0.9.45, FLEXOR: K1 THRU, K2 HOME's break
                                          * filter (as the rolls') until 0.9.63, its LATCH since (FLEXOR), K3 STOP, K4
                                          * SPIN UP) */
        if (k == 0) { text_cat(t, THRU_NAME[set.vinyl & 3u]); break; }
        if (k == 1) goto lat;
        signed_tenths(k == 3 ? p->spin_steps : p->stop_beats, k == 3 ? " ST" : " BT", t);
        if (t[0] == '+') t[0] = ' ';
        break;
    case PAD_GATE:
        if (k == 3) goto lat;
        if (k == 0) text_cat(t, BRK_GATE_DIV_NAME[p->gate_div % BRK_GATE_DIVS]);
        else pct_text(p->gate_len, t);
        break;
    case PAD_RANDOM:
        if (k == 3) goto lat;
        if (k == 0) pct_text(p->rnd_amount, t);
        else { int_text((int32_t)p->rnd_pitch, t); text_cat(t, "ST"); }
        break;
    case PAD_FILTER:
        if (k == 0) dj_text(f->flt_target, t);
        else if (k == 1) pct_text(f->flt_res, t);
        else if (k == 2) pct_text(f->flt_speed, t);
        else text_cat(t, FTHRU_NAME[bk_fthru()]);
        break;
    case PAD_FREEZE:                     /* (LOAF BREAKER, 11 Nov 2026: CHOPPAH's, back) */
        if (k == 3) goto lat;
        if (k == 0) text_cat(t, FRZ_LEN_NAME[p->frz_len & 3u]);
        else signed_tenths(p->frz_pitch, "ST", t);
        break;
    case PAD_DUB:                        /* with a DELAY in the chain, the pad throws into it: its settings */
        if (delay_slot() >= 0) fx_value((uint32_t)delay_slot(), k, t);
        else if (k == 0) text_cat(t, BRK_DUB_DIV_NAME[f->dub_div % BRK_DUB_DIVS]);
        else pct_text(k == 1 ? f->dub_fb : k == 2 ? f->dub_tone : f->dub_mix, t);
        break;
    case BKF_ARP: {                      /* the ARP key: MAIN or MORE (pad_knob_name); the one held last's */
        uint32_t e = arpk_fx(arpk_focus());
        int tk = ovl_time_kind(e);
        if (ui.pad_more) {
            if (k == 0) text_cat(t, ARP_THRU[arp_route(e)]);
            else if (k == 1) text_cat(t, set.ovl_pad & 32u ? "OFF" : "ON");
            else if (k == 2) text_cat(t, OVL_RATE_NAME[set.ovl_rate & 3u]);
            else text_cat(t, OVL_SWEEP_NAME[set.ovl_sweep & 3u]);
        } else if (k == 0)
            text_cat(t, OVL_NAME[e]);
        else if (k == 1) {
            int_text(set.ovl_amt[e], t);
            text_cat(t, "%");
        } else if (k == 2)
            text_cat(t, tk == 1 ? OVL_RATE_NAME[set.ovl_rate & 3u] : tk == 2 ? OVL_SWEEP_NAME[set.ovl_sweep & 3u] : "-");
        else
            text_cat(t, arpk_latch(arpk_focus()) ? "ON" : "OFF");
        break;
    }
    default: break;
    }
    return;
lat:                                     /* (K4 LATCH: ON, OFF) */
    text_cat(t, bk_lat() >> lat_bit((uint32_t)pad) & 1u ? "ON" : "OFF");
}

/* THRU's knob (the ARP key's, the VINYL BREAK's): left to right KICK, BREAK, BOTH; code 0 BOTH, 1 BREAK, 2 KICK */
static uint32_t thru_turn(uint32_t code, int32_t d)
{
    static const uint8_t AT[3] = {2u, 1u, 0u}, POS[4] = {2u, 1u, 0u, 0u};   /* place <-> code */
    return AT[clamp((int32_t)POS[code & 3u] + (d > 0 ? 1 : -1), 0, 2)];
}

static void pad_knob_turn(int pad, int k, int32_t d)
{
    brk_padset_t *p = &set.pads;
    brk_fx_t *f = &set.fx;
    char t[16];
    if (pad < 0 || pad >= (int)BKF_N || k < 0 || k > 3 || !pad_knob_name(pad, k))
        return;
    switch (pad <= PAD_ROLL32 ? 0 : pad) {
    case 0:
        if (k == 0) {                    /* (RATE: longest to shortest, the triplets last: BRK_ROLL_ORDER) */
            int32_t o = 0;
            while (o < (int32_t)BRK_ROLL_DIVS - 1 && BRK_ROLL_ORDER[o] != p->roll_div[pad] % BRK_ROLL_DIVS)
                o++;
            p->roll_div[pad] = BRK_ROLL_ORDER[clamp(o + d, 0, BRK_ROLL_DIVS - 1)];
        }
        else if (k == 1) {               /* (0.9.40, FLEXOR: K2 the break's filter, HOME's, rolling) */
            brk_flt_turn(d);
            return;
        } else if (k == 2) {             /* K3 the drums' filter: HOME's (and SEQ's) */
            kick_flt_turn(d);
            return;
        }
        else p->roll_decay = knob_f(p->roll_decay, d, 0.02f, -0.9f, 0.9f);
        break;
    case BKF_KBR:                        /* (0.9.58: KILL BREAK) */
        if (k == 0)
            kb_put((uint32_t)clamp((int32_t)kb_mode() + (d > 0 ? 1 : -1), 0, KB_N - 1), kb_latch(), kb_time(), kb_rev());
        else if (k == 1 && kb_mode() == KB_BAR)   /* (0.9.60: BAR END's ECHO) */
            kb_echo_put((uint32_t)clamp((int32_t)kb_echo() + (d > 0 ? 1 : -1), 0, KB_ECHOS - 1));
        else if (k == 1)
            kb_put(kb_mode(), kb_latch(), (uint32_t)clamp((int32_t)kb_time() + (d > 0 ? 1 : -1), 0, KB_TIMES - 1), kb_rev());
        else if (k == 2)
            kb_put(kb_mode(), kb_latch(), kb_time(), (uint32_t)clamp((int32_t)kb_rev() + d * 5, 0, 100));
        else
            kb_put(kb_mode(), d > 0 ? 1u : 0u, kb_time(), kb_rev());
        kbrk_mode = (uint8_t)kb_mode();
        kbrk_t16 = KB_T16[kb_time()];
        kbrk_rev = (uint8_t)kb_rev();
        kbrk_echo = (uint8_t)(kb_mode() == KB_BAR ? kb_echo() : 0u);
        break;
    case PAD_REVERSE:
        if (k == 3) goto lat;
        p->rev_speed = knob_f(p->rev_speed, d, 0.05f, 0.5f, 2.0f);
        break;
    case PAD_HALF:                       /* GLOUPI BREAKS: K2 MASTER TEMPO, right on (the pitch kept), left off */
        if (k == 0) p->half_speed = knob_f(p->half_speed, d, 0.05f, 0.25f, 0.75f);
        else if (k == 1) set.half_mt_off = (uint8_t)((set.half_mt_off & ~1u) | (d < 0 ? 1u : 0u));
        else {                           /* LOAF BREAKER: K3 LATCH, right on (held now: it stays on), left off */
            set.half_mt_off = (uint8_t)(d > 0 ? set.half_mt_off | 2u : set.half_mt_off & ~2u);
            half_lat = (uint8_t)(d > 0 && ui.pad_focus == PAD_HALF);
        }
        break;
    case PAD_TAPESTOP:                   /* (0.9.45, FLEXOR: K1 THRU, K2 the break's filter (0.9.64, FLEXOR: its LATCH),
                                          * K3 STOP, K4 SPIN UP) */
        if (k == 1)
            goto lat;
        if (k == 2)                      /* (LOAF BREAKER: to 16 beats, a beat a detent past 4) */
            p->stop_beats = knob_f(p->stop_beats, d, p->stop_beats > 4.0f || (p->stop_beats > 3.9f && d > 0) ? 1.0f
                                                                                                             : 0.25f,
                                   0.25f, 16.0f);
        else if (k == 3) p->spin_steps = knob_f(p->spin_steps, d, 0.25f, 0.25f, 8.0f);
        else set.vinyl = (uint8_t)thru_turn(set.vinyl, d);
        break;
    case PAD_GATE:
        if (k == 3) goto lat;
        if (k == 0) p->gate_div = (uint8_t)clamp((int32_t)p->gate_div + d, 0, BRK_GATE_DIVS - 1);
        else p->gate_len = knob_f(p->gate_len, d, 0.05f, 0.1f, 0.9f);
        break;
    case PAD_RANDOM:
        if (k == 3) goto lat;
        if (k == 0) p->rnd_amount = knob_f(p->rnd_amount, d, 0.05f, 0.0f, 1.0f);
        else p->rnd_pitch = knob_f(p->rnd_pitch, d, 1.0f, 0.0f, 12.0f);
        break;
    case PAD_FILTER:
        if (k == 0) f->flt_target = knob_f(f->flt_target, d, 0.04f, -1.0f, 1.0f);
        else if (k == 1) f->flt_res = knob_f(f->flt_res, d, 0.02f, 0.0f, 0.95f);
        else if (k == 2) f->flt_speed = knob_f(f->flt_speed, d, 0.02f, 0.0f, 1.0f);
        else                             /* (0.9.26: K4 THRU, left to right BREAK, DRUMS, BOTH) */
            set.bkeys.bk[BK_FTHRU] = (uint8_t)(0xC0u | (uint32_t)clamp((int32_t)bk_fthru() + (d > 0 ? 1 : -1), 0, 2));
        break;
    case PAD_FREEZE:
        if (k == 3) goto lat;
        if (k == 0) p->frz_len = (uint8_t)clamp((int32_t)p->frz_len + d, 0, 3);
        else p->frz_pitch = knob_f(p->frz_pitch, d, 1.0f, -12.0f, 12.0f);
        break;
    case PAD_DUB:
        if (delay_slot() >= 0) {         /* the DELAY slot's own settings */
            fx_knob_turn((uint32_t)delay_slot(), k, d);
            return;
        }
        if (k == 0) f->dub_div = (uint8_t)clamp((int32_t)f->dub_div + d, 0, BRK_DUB_DIVS - 1);
        else if (k == 1) f->dub_fb = knob_f(f->dub_fb, d, 0.02f, 0.0f, 0.95f);
        else if (k == 2) f->dub_tone = knob_f(f->dub_tone, d, 0.02f, 0.0f, 1.0f);
        else f->dub_mix = knob_f(f->dub_mix, d, 0.02f, 0.0f, 1.0f);
        break;
    case BKF_ARP:                        /* GLOUPI BREAKS: the ARP key. K1 which OVERLAY effect (the ARP page's
                                          * settings: amount, rate, sweep), K2 THRU, K3 ON / OFF, K4 LATCH */
        {
            uint32_t e = arpk_fx(arpk_focus());  /* (0.9.45: the ARP key held last's own) */
            int tk = ovl_time_kind(e), dir = d > 0 ? 1 : -1;
            if (ui.pad_more) {    /* MORE: THRU, ON / OFF, RATE, SWEEP */
                if (k == 0)              /* (THRU: this effect's, app.c's render) */
                    arp_thru_turn(e, d);
                else if (k == 1)         /* (ON: the key sounds its effect; OFF: K1 browses them silent) */
                    set.ovl_pad = (uint8_t)(d > 0 ? set.ovl_pad & ~32u : set.ovl_pad | 32u);
                else if (k == 2)
                    set.ovl_rate = (uint8_t)clamp((int32_t)set.ovl_rate + dir, 0, 3);
                else
                    set.ovl_sweep = (uint8_t)clamp((int32_t)set.ovl_sweep + dir, 0, 3);
            } else if (k == 0) {         /* MAIN: K1 the effect (the OVERLAY page's K1 follows it: ovl_last) */
                e = (e + (d > 0 ? 1u : 15u)) & 15u;
                arpk_set_fx(arpk_focus(), e);
                set.ovl_pad = (uint8_t)((set.ovl_pad & ~15u) | e);
            }
            else if (k == 1)             /* K2 its amount */
                set.ovl_amt[e] = (uint8_t)clamp((int32_t)set.ovl_amt[e] + d * 2, 0, 100);
            else if (k == 2) {           /* K3 its time: RATE or SWEEP, as the effect has */
                if (tk == 1)
                    set.ovl_rate = (uint8_t)clamp((int32_t)set.ovl_rate + dir, 0, 3);
                else if (tk == 2)
                    set.ovl_sweep = (uint8_t)clamp((int32_t)set.ovl_sweep + dir, 0, 3);
                else
                    return;
            } else {                     /* K4 LATCH, right on (each press on or off), left off */
                arpk_set_latch(arpk_focus(), d > 0);
                if (d < 0)
                    arp_lat &= (uint16_t)~(1u << arpk_focus());   /* (latched on: off with it) */
            }
            set_dirty = 1u;
        }
        ovl_push();
        ui.redraw |= RG_FOOT;
        ui.redraw |= RG_BLOCKS;
        break;
    default: break;
    }
    if (0) {
lat:                                     /* (0.9.26: K4 LATCH, right on (held now: it stays on), left off; as HALF's) */
        bk_lat_put(d > 0 ? bk_lat() | 1u << lat_bit((uint32_t)pad) : bk_lat() & ~(1u << lat_bit((uint32_t)pad)));
        if (d > 0 && ui.pad_focus == pad)
            pad_latched |= (uint16_t)(1u << pad);
    }
    pad_knob_value(pad, k, t);
    app_say(pad_knob_name(pad, k), t);
    changed();
}

/* ---------------------------------------------------------- pad animations --- */
#define C_ICE RGB(150, 220, 255)
#define C_DUB RGB(120, 230, 160)
#define FX_Y 90                          /* the animation's strip: 240 x 96 where the slice blocks are */
#define FX_MID 58                        /* its waveform's centre line, in the strip */
#define FX_BAND 48                       /* drawn in two bands of 48 rows (the canvas holds 60) */

static uint32_t fx_hash(uint32_t x)      /* a stable pseudo-random number from x (the screen's dice) */
{
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    return x ^ (x >> 16);
}

static void fx_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* a circle of dots */
{
    uint32_t a;
    for (a = 0; a < 64u; a++) {
        float t = (float)a / 64.0f;
        cv_rect(cx + (int32_t)((float)r * brk_sin_turns(t + 0.25f)), cy + (int32_t)((float)r * brk_sin_turns(t)),
                2, 2, c);
    }
}

static void fx_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)   /* LOAF BREAKER: a filled disc */
{
    int32_t dy, w = r;
    for (dy = 0; dy <= r; dy++) {
        while (w * w + dy * dy > r * r)
            w--;
        cv_rect(cx - w, cy - dy, 2 * w + 1, 1, c);
        cv_rect(cx - w, cy + dy, 2 * w + 1, 1, c);
    }
}

static void fx_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t n = (x1 > x0 ? x1 - x0 : x0 - x1) > (y1 > y0 ? y1 - y0 : y0 - y1) ? (x1 > x0 ? x1 - x0 : x0 - x1)
                                                                            : (y1 > y0 ? y1 - y0 : y0 - y1), i;
    for (i = 0; i <= n; i++)
        cv_rect(x0 + (x1 - x0) * i / (n ? n : 1), y0 + (y1 - y0) * i / (n ? n : 1), 2, 2, c);
}

/* a slice's shape (blk_px) stretched over w columns from x, h times as tall, in colour c */
static void fx_slice(int32_t s, int32_t x, int32_t w, float h, uint16_t c)
{
    int32_t i;
    if (s < 0 || s >= (int32_t)blk_n)
        return;
    for (i = 0; i < w; i++) {
        int32_t v = (int32_t)((float)blk_px[s][(uint32_t)i * BLK_W / (uint32_t)w] * 1.7f * h);
        cv_rect(x + i, FX_MID - v, 1, 2 * v + 1, c);
    }
}

/* the held pad's picture: what it does to the break, moving with it, and its settings */
static void draw_fx_band(int pad, const brk_status_t *st, uint32_t now, int32_t top)
{
    const brk_padset_t *p = &set.pads;
    const brk_fx_t *f = &set.fx;
    char t[24];
    int32_t i, s = st->slice >= 0 ? st->slice : 0;
    uint32_t held = now - ui.pad_since;
    cv_begin(240, FX_BAND, C_BLACK);
    cv_oy = -top;                        /* (this band's rows of the picture) */
    t[0] = 0;
    if (pad <= PAD_ROLL32 && (roll_sel || set.pads.rsv[0] % 3u))   /* (LOAF BREAKER: THRU KICK or BOTH, or SEL) */
        text_cat(t, roll_sel || set.pads.rsv[0] % 3u == 1u ? "KICK " : "ALL ");
    text_cat(t, PAD_TITLE[pad]);
    if (pad <= PAD_ROLL32) {
        text_cat(t, " ");
        text_cat(t, BRK_ROLL_DIV_NAME[p->roll_div[pad] % BRK_ROLL_DIVS]);
    }
    cv_text(6, 0, &FONT_B, t, C_HI);
    switch (pad <= PAD_ROLL32 ? 0 : pad) {
    case 0: {                            /* ROLL: the caught slice again and again at its level, fading or
                                          * swelling, the hit lit */
        int32_t rs = st->roll_slice >= 0 ? st->roll_slice : s, cur = (int32_t)(st->roll_n % 8u);
        float g = (float)(p->roll_level ? p->roll_level : 100u) * 0.01f;
        for (i = 0; i < 8; i++) {
            fx_slice(rs, 4 + i * 29, 27, g, i == cur ? C_HI : C_DIM);
            g = p->roll_decay >= 0.0f ? g * (1.0f - p->roll_decay) : g + (1.0f - g) * -p->roll_decay;
            if (g < 0.02f)
                g = 0.02f;
        }
        int_text((int32_t)st->roll_n, t);
        text_cat(t, " HITS");
        cv_text(234 - text_w(&FONT_XS, t), 2, &FONT_XS, t, C_GRAY);
        break;
    }
    case BKF_KBR: {                      /* (0.9.58: KILL BREAK: its shape, the break's level falling as its MODE says) */
        static const char *const KD[KB_N] = {"THE BREAK FADES OUT", "THE BREAK CUT AT ONCE", "OUT AT THE BAR'S END,",
                                             "TO ITS LOOP'S END, THEN FADES OUT"};
        uint32_t mo = kb_mode();
        for (i = 0; i < 200; i++) {      /* (its level over time: down the ramp, the step, or at the bar's end) */
            int32_t h = mo == KB_FADE ? 24 - i * 24 / 200 : mo == KB_CUT ? (i < 20 ? 24 : 0) : mo == KB_ENDF
                        ? (i < 100 ? 24 : 24 - (i - 100) * 24 / 100) : (i < 150 ? 24 : 0);
            cv_rect(20 + i, FX_MID + 6 - h, 1, h > 0 ? h : 1, h > 0 ? C_HI : C_LINE);
        }
        if (mo == KB_BAR)                /* (the reverb's tail after it) */
            for (i = 150; i < 220; i++)
                cv_rect(20 + i, FX_MID + 6 - (220 - i) / 7, 1, (220 - i) / 7 + 1, C_DIM);
        cv_text(20, FX_MID + 11, &FONT_XS, KD[mo], C_GRAY);
        if (mo == KB_BAR)
            cv_text(20, FX_MID + 22, &FONT_XS, kb_echo() ? "ITS LAST 8TH INTO THE ECHO AND REVERB" : "ITS LAST 8TH INTO THE REVERB", C_GRAY);
        cv_text(234 - text_w(&FONT_XS, kbrk_hold ? "OUT" : "BACK"), 2, &FONT_XS, kbrk_hold ? "OUT" : "BACK",
                kbrk_hold ? C_HI : C_GRAY);
        break;
    }
    case PAD_REVERSE: {                  /* REVERSE: the break running backwards past a fixed playhead, chevrons */
        int32_t off = (int32_t)((float)held * 0.12f * p->rev_speed) % 240;
        for (i = 0; i < 240; i++) {
            int32_t h = wave_px[(uint32_t)(240 + off - i) % 240u] * 3 / 2;
            cv_rect(i, FX_MID - h, 1, 2 * h + 1, i < 120 ? C_DIM : C_HI);
        }
        cv_rect(120, 20, 2, 76, C_WHITE);
        for (i = 0; i < 2; i++) {        /* two chevrons pointing left, sliding */
            int32_t cx = 40 + i * 22 - (int32_t)(held / 40u % 22u), r;
            for (r = 0; r < 9; r++)
                cv_rect(cx + r, FX_MID - r - 1, 3, 2, C_WHITE), cv_rect(cx + r, FX_MID + r, 3, 2, C_WHITE);
        }
        break;
    }
    case PAD_HALF: {                     /* HALF SPEED: the break stretched, crawling */
        int32_t off = (int32_t)((float)held * 0.05f * p->half_speed);
        for (i = 0; i < 240; i++) {
            int32_t h = wave_px[(uint32_t)((i + off) / 2) % 240u] * 3 / 2;
            cv_rect(i, FX_MID - h, 1, 2 * h + 1, (i / 2) % 2 ? C_HI : C_AMB);
        }
        times_text(p->half_speed, t);
        cv_text(234 - text_w(&FONT_B, t), 0, &FONT_B, t, C_WHITE);
        break;
    }
    case PAD_GATE: {                     /* GATE: the bar's gates, open part lit, the one playing bright */
        int32_t cells = 8 << (p->gate_div % BRK_GATE_DIVS), w = 236 / cells, curc;
        float pos = (float)(st->step < 0 ? 0 : st->step % 16) + st->phase;
        curc = (int32_t)(pos * (float)cells / 16.0f) % cells;
        for (i = 0; i < cells; i++) {
            int32_t open = (int32_t)((float)(w - 1) * p->gate_len);
            cv_rect(2 + i * w, 34, w - 1, 48, C_BLOCK);
            cv_rect(2 + i * w, 34, open < 1 ? 1 : open, 48, i == curc ? (st->gate > 0.5f ? C_WHITE : C_HI) : C_AMB);
        }
        break;
    }
    case PAD_RANDOM: {                   /* RANDOM: dice over the slices, the one sounding big */
        uint32_t seed = (uint32_t)(st->step + 1) * 2654435761u;
        for (i = 0; i < 16; i++) {
            int32_t x = 120 + (i % 8) * 15, y = 30 + (i / 8) * 32;
            uint32_t r = fx_hash(seed + (uint32_t)i);
            cv_rect(x, y, 13, 28, i == s ? C_HI : (r & 3u) == 0u ? C_GRAY : C_BLOCK);
        }
        int_text(s + 1, t);
        cv_text2(20, 30, &FONT_B, t, C_WHITE);
        if (p->rnd_pitch > 0.0f)
            cv_text(20, 72, &FONT_T, "+ PITCH", C_AMB);
        break;
    }
    case PAD_FILTER: {                   /* FILTER: the response curve, its cutoff sliding to the target */
        float dj = brk_fx_dj_now(), xc = dj > 0.0f ? dj * 220.0f + 10.0f : (1.0f + dj) * 220.0f + 10.0f;
        float res = f->flt_res;
        for (i = 0; i < 240; i++) {
            float x = (float)i, d = dj > 0.0f ? xc - x : x - xc, y;
            y = d < 0.0f ? 1.0f : 1.0f / (1.0f + d * d * 0.004f);   /* past the cutoff: falls away */
            if (d > -12.0f && d < 12.0f)
                y += res * 0.8f * (1.0f - brk_absf(d) / 12.0f);     /* the resonance's bump */
            {
                int32_t h = (int32_t)(y * 34.0f);
                cv_rect(i, 92 - h, 1, h, i % 2 ? C_AMB : C_HI);
            }
        }
        cv_rect((int32_t)xc, 22, 2, 72, C_WHITE);
        dj_text(dj, t);
        cv_text(234 - text_w(&FONT_XS, t), 2, &FONT_XS, t, C_WHITE);
        break;
    }
    case PAD_FREEZE: {                   /* FREEZE: the slice iced over, its loop bracketed, a cursor going round */
        int32_t loop = 240 >> (p->frz_len & 3u), cx;
        fx_slice(s, 0, 240, 1.0f, C_LINE);
        fx_slice(s, 0, 240, 1.0f, C_ICE);
        cv_rect(0, 22, loop, 2, C_WHITE);
        cv_rect(0, 92, loop, 2, C_WHITE);
        cv_rect(loop - 2, 22, 2, 72, C_WHITE);
        cx = (int32_t)(held % 700u) * loop / 700;
        cv_rect(cx, 22, 2, 72, C_HI);
        for (i = 0; i < 24; i++) {       /* frost: dots that twinkle */
            uint32_t r = fx_hash((uint32_t)i * 977u + held / 180u);
            cv_rect((int32_t)(r % 236u), 24 + (int32_t)((r >> 8) % 66u), 2, 2, (r >> 20) & 1u ? C_WHITE : C_ICE);
        }
        break;
    }
    case PAD_DUB: {                      /* DUB: echoes marching out, each one fainter and darker */
        static const uint8_t SP[BRK_DUB_DIVS] = {2, 3, 4, 6};
        float tempo = (float)set.tempo_x10 * 0.1f, step_ms = 60000.0f / (tempo * 4.0f);
        float per = step_ms * (float)SP[f->dub_div % BRK_DUB_DIVS], ph = (float)(held % (uint32_t)per) / per;
        int32_t sp = 26 + 8 * (int32_t)SP[f->dub_div % BRK_DUB_DIVS] / 2;
        float g = 1.0f;
        for (i = 0; i < 10; i++) {
            int32_t x = 6 + i * sp + (int32_t)(ph * (float)sp);
            if (x > 236)
                break;
            fx_slice(s, x, 18, g, g > 0.6f ? C_DUB : g > 0.3f ? C_GRAY : C_LINE);
            g *= f->dub_fb;
        }
        break;
    }
    case BKF_ARP: {                      /* GLOUPI BREAKS: the ARP key: its effect, big, lit while on */
        uint32_t e = arpk_fx(arpk_focus());
        int on = arpk_on(arpk_focus());
        const char *n = OVL_NAME[e];
        cv_rect(6, 30, 228, 40, on ? C_HI : C_BLOCK);
        cv_text((240 - text_w(&FONT_B, n)) / 2, 38, &FONT_B, n, on ? C_BLACK : C_GRAY);
        cv_rect(6, 72, 228, 3, C_BLOCK); /* (LOAF BREAKER: its amount) */
        cv_rect(6, 72, (int32_t)set.ovl_amt[e] * 228 / 100, 3, on ? C_HI : C_GRAY);
        cv_text(6, 78, &FONT_XS, set.ovl_pad & 32u ? "OFF (FX: MORE, K2): K1 BROWSES, SILENT"
                                 : arpk_latch(arpk_focus()) ? "LATCH: EACH PRESS ON OR OFF" : "ON WHILE HELD", C_DIM);
        if (arpk_count() > 1u) {         /* (0.9.45: several ARP keys: which one, each its own effect) */
            char k[12] = "KEY ";
            int_text((int32_t)arpk_focus() + 1, k + 4);
            cv_text(234 - text_w(&FONT_XS, k), 16, &FONT_XS, k, C_HI);   /* (under FX: MORE, the title left) */
        }
        cv_text(234 - text_w(&FONT_XS, ui.pad_more ? "MORE (FX: MAIN)" : "FX: MORE"), 4, &FONT_XS,
                ui.pad_more ? "MORE (FX: MAIN)" : "FX: MORE", ui.pad_more ? C_HI : C_DIM);
        (void)held;
        break;
    }
    default: break;
    }
    cv_oy = 0;
    cv_blit(0, FX_Y + (uint32_t)top);
}

/* LOAF BREAKER (0.9.30, FLEXOR: "le plateau géant décalé", the start-up picture's face on its label): VINYL BREAK held,
 * a Technics platter over the wave and the middle (screen rows 44-187), to the left, cut by the screen: its rim's four
 * rows of strobe dots (the 33's lit: still at full speed, the others creeping; all racing back as it slows), the record,
 * its grooves, the face turning with it; the speed and its semitones in a column on the right. main.c leaves the wave
 * alone meanwhile and draws it again when it goes */
#define VN_CX 70
#define VN_CY 72                         /* (its picture's rows: screen 44 + y) */
#define VN_R 110
static float vn_rows[4];                 /* each strobe row's drift, in dots */
static void vn_circle(int32_t r, float off, int32_t step, uint16_t c, int32_t top, int32_t sz)   /* dots round a circle */
{
    int32_t n = r * 6283 / (step * 1000), d;
    float cs, sn, cd = brk_sin_turns(0.25f + 1.0f / (float)n), sd = brk_sin_turns(1.0f / (float)n), t;
    cs = brk_sin_turns(0.25f + off / (float)n);
    sn = brk_sin_turns(off / (float)n);
    for (d = 0; d < n; d++) {
        int32_t y = VN_CY + (int32_t)((float)r * sn);
        if (y >= top - 2 && y < top + FX_BAND + 2)
            cv_rect(VN_CX + (int32_t)((float)r * cs), y, sz, sz, c);
        t = cs * cd - sn * sd;           /* (the next dot: turned by one step) */
        sn = sn * cd + cs * sd;
        cs = t;
    }
}
static void vn_band(const brk_status_t *st, int32_t top)
{
    float tape = st->tape, ca = brk_sin_turns(0.25f - ui.reel), sa = brk_sin_turns(-ui.reel);
    int32_t r, x, y;
    char t[16];
    cv_begin(240, FX_BAND, C_BLACK);
    cv_oy = -top;
    fx_disc(VN_CX, VN_CY, VN_R + 2, C_BLOCK);
    for (r = 0; r < 4; r++)
        vn_circle(VN_R - 2 - r * 4, vn_rows[r], 4, r == 1 ? C_HI : C_GRAY, top, 2);
    fx_disc(VN_CX, VN_CY, VN_R - 18, C_DIM);   /* the mat's edge, the record, its grooves */
    fx_disc(VN_CX, VN_CY, VN_R - 21, C_BLACK);
    for (r = VN_R - 25; r > 40; r -= 5)
        vn_circle(r, 0.0f, 3, C_LINE, top, 1);
    fx_disc(VN_CX, VN_CY, 36, C_HI);     /* the label: the face, turning with the record */
    for (y = VN_CY - 32; y < VN_CY + 32; y++) {
        if (y < top || y >= top + FX_BAND)
            continue;
        for (x = -32; x < 32; x++) {
            int32_t dy = y - VN_CY, sx = (int32_t)(ca * (float)x - sa * (float)dy) + 32,
                    sy = (int32_t)(sa * (float)x + ca * (float)dy) + 32;
            uint32_t v;
            if ((uint32_t)sx >= FACE_N || (uint32_t)sy >= FACE_N)
                continue;
            v = FACE_PX[((uint32_t)sy * FACE_N + (uint32_t)sx) >> 1];
            v = sx & 1 ? v & 15u : v >> 4;
            if (v)
                cv_pset(VN_CX + x, y, (uint16_t)((v * 2u) << 11 | (v * 4u) << 5 | v * 2u));
        }
    }
    fx_disc(VN_CX, VN_CY, 2, C_BLACK);
    cv_rect(VN_CX + (int32_t)(68.0f * brk_sin_turns(0.25f + ui.reel * 3.0f)) - 1,
            VN_CY + (int32_t)(68.0f * brk_sin_turns(ui.reel * 3.0f)) - 1, 3, 3, C_WHITE);   /* a glint in the grooves */
    cv_rect(178, 0, 62, VN_CY * 2, C_BLACK);   /* the column: the speed, its semitones, the record, THRU */
    cv_rect(178, 0, 1, VN_CY * 2, C_LINE);
    int_text((int32_t)(tape * 100.0f + 0.5f), t);
    text_cat(t, "%");
    cv_text2(236 - text2_w(&FONT_B, t), 6, &FONT_B, t, tape < 0.02f ? C_HI : C_WHITE);
    t[0] = 0;
    if (tape > 0.004f)
        signed_tenths(12.0f * brk_log2f(tape), " ST", t);
    else
        text_cat(t, "STOP");
    cv_text(236 - text_w(&FONT_B, t), 40, &FONT_B, t, C_HI);
    cv_text(236 - text_w(&FONT_XS, "33 1/3 RPM"), 112, &FONT_XS, "33 1/3 RPM", C_GRAY);
    cv_text(236 - text_w(&FONT_XS, THRU_NAME[set.vinyl & 3u]), 126, &FONT_XS, THRU_NAME[set.vinyl & 3u], C_GRAY);
    cv_oy = 0;
    cv_blit(0, 44u + (uint32_t)top);
}
static void draw_vinyl(const brk_status_t *st, uint32_t now)
{
    static const float CREEP[4] = {0.6f, 0.0f, -0.5f, 1.1f};   /* (the 45's, 50 / 60 Hz rows creep; the 33's still) */
    static uint32_t last;
    float dt = (float)(now - last > 100u ? 33u : now - last) * 0.001f;
    int32_t r;
    last = now;
    for (r = 0; r < 4; r++) {
        vn_rows[r] += (CREEP[r] * st->tape - (1.0f - st->tape) * 24.0f) * dt;
        if (vn_rows[r] < -1000.0f || vn_rows[r] > 1000.0f)
            vn_rows[r] = 0.0f;
    }
    for (r = 0; r < 3; r++)
        vn_band(st, r * FX_BAND);
}

/* (drawn in two bands of FX_BAND rows: the canvas is 60 rows) */
static void draw_fx(int pad, const brk_status_t *st, uint32_t now)
{
    if (pad == PAD_TAPESTOP) {
        draw_vinyl(st, now);
        return;
    }
    draw_fx_band(pad, st, now, 0);
    draw_fx_band(pad, st, now, FX_BAND);
}

/* GLOUPI BREAKS: the ARP black key (BKF_ARP; LOAF BREAKER, 11 Nov 2026: DUB back beside it): an OVERLAY effect, held
 * or switched on, over what the ARP page has on, with the ARP page's settings (ovl.c); 0.9.45: each ARP key its own
 * (arpk_*, above). On: any of them */
static int ovl_pad_on(void)
{
    uint32_t j;
    for (j = 0; j < 11u; j++)
        if (arpk_on(j))
            return 1;
    return 0;
}


/* LOAF BREAKER (11 Nov 2026): a black key's function lit: a pad on (or rolling the kick), the ARP key's effect on,
 * the kicks held quiet, the DISTO switch on, its SUB FX switch on (0.244's BEGINNER restrictions gone: FLEXOR) */
static int bkf_lit(uint32_t f, const brk_status_t *st)
{
    if (f < BRK_NPADS)
        return (int)((st->pads >> f) & 1u) || roll_lit(f);
    if (f == BKF_ARP)
        return ovl_pad_on();
    if (f == BKF_KIL)
        return kick_hold != 0u;
    if (f == BKF_DST)                    /* (the switch on, even with the DISTO at 0 %: 0.268, FLEXOR) */
        return !terror_off;
    if (f == BKF_DRV)                    /* (0.9.43: the DRIVE switch on, even at 0 %) */
        return !drive_off;
    if (f == BKF_X2)                     /* (0.9.48: the kicks doubled) */
        return (kick_x2 & 1u) != 0u;
    if (f == BKF_X4)                     /* (0.9.55: on every step) */
        return (kick_x2 & 2u) != 0u;
    if (f == BKF_TRI)                    /* (0.9.55: in triplets) */
        return (kick_x2 & 4u) != 0u;
    if (f == BKF_KBR)                    /* (0.9.58: KILL BREAK, the break out) */
        return kbrk_hold != 0u;
    if (f == BKF_CHS)                    /* (0.9.76: CHAOS heard: the switch on and something set) */
        return !chaos_off && kx2.ch_tag == CH_TAG && (kx2.chaos[0] || kx2.chaos[1] || kx2.chaos[2]);
    if (f == BKF_SDR)                    /* (0.9.57: SUB DRIVE, the SUB layer's effect DRIVE, heard) */
        return set.kick.l[KL_SUB].fx == KFX_DRIVE && !subfx_off;
    return !subfx_off;                   /* (0.9.43: SUB, the SUB FX switch on, even with no effect set) */
}
