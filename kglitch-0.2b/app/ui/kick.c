/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the KICK page (EDIT): the kick's layers, effects, macros and output. (Part of app.c's unity: included
 * there in order.) */

/* ---- the KICK page (PRD K-*): EDIT opens it, EDIT again the next sub-page. The white keys are the kick's 16
 * steps, ALGORITHM the layer, PRESETS the built-in kicks, SEL + EDIT through the FX chain or around it */
enum { KP_MIX, KP_LAYER, KP_FX, KP_MACRO, KP_OUT, KP_TERROR, KP_TMORE, KP_TERROR2, KP_SYN1, KP_SYN2, KP_BASS, KP_KNOBS,
       KP_KSET, KP_CMORPH, KP_STYLE, KP_TAIL, KP_TAIL2, KP_STACK, KP_TOOLS, KP_PRESET, KP_TOK, KP_GATE, KPAGES };   /* (KP_TOK,
                                          * KP_GATE: 0.9.45, FLEXOR's Kick Lab 2) */   /* (KP_PRESET: LOAF
                                          * BREAKER 0.253, FLEXOR: ATTACK's first part, the built-in kicks alone) */
/* LOAF BREAKER: the KICK LAB, the menu's LAB section (klab in app.c, kick.h kick_lab_t): TAIL (K1 MODE, K2 NOTE, K3
 * CURVE, K4 END), TAIL + (K1 FILTER, K2 F TIME, K3 DRIVE), STACK (K1 ATTACK, K2 CROSS, K3 FADE, K4 SOLO), TOOLS (K1
 * MUTATE, K2 A/B) */
static int klab_page(void) { return ui.kpage >= KP_TAIL && ui.kpage <= KP_TOOLS; }
static void klab_value(int i, char *t);
static float klab_norm(int i);
static void klab_turn(int i, int32_t d);
static void ab_take(void);
/* (LOAF BREAKER TEST: TERROR, TERROR +, TERROR 2 the kick's own settings of them, ENV's KNOB functions 3, 4, 1) */
static void selk_turn(uint32_t f, int i, int32_t d);
static void selk_value(uint32_t f, int i, char *t);
static float sk_norm(uint32_t f, int i);
static uint32_t selk_fn(uint32_t knob);
static const char *sk_short(uint32_t f);
static const char *sk_kname(uint32_t f, int i);
/* LOAF BREAKER: the KNOBS section: KNOBS, K1-K4 what SEL + K1..K4 turns (each function's first setting), with the
 * kick's picture; K1 SET .. K4 SET (ui.kks), that knob's function's settings, as ENV KNOB. The kick playing's (each
 * kick memory its own: steps.kx) */
static int kp_knobs(void) { return ui.kpage == KP_KNOBS || ui.kpage == KP_KSET; }
static uint32_t kn_fn(int i) { return selk_fn(ui.kpage == KP_KSET ? ui.kks & 3u : (uint32_t)(i & 3)); }
static int kn_i(int i) { return ui.kpage == KP_KSET ? i : 0; }
/* LOAF BREAKER: SYNTH's CLEAN · MORPH part: K1 CLEAN SUB, K2 MORPH, K3 its TO KICK (ENV KNOB's functions 13, 14: their
 * own place, the kick's own, with no knob given them) */
static uint32_t cm_fn(int i) { return i == 0 ? 13u : 14u; }
/* LOAF BREAKER: KNOBS' STYLE part: K1-K4 each its knob's function (one at a time), PRESETS a style (TERROR, EARLY NEXT
 * GEN, HARDCORE ..) giving the four SEL + knobs their functions at once (the kick playing's); track.c has them */
static void kstyle_turn(int32_t d);
static void selk_assign_k(uint32_t kn, int32_t d);
static const char *kstyle_name(void);
static int cm_i(int i) { return i == 2 ? 1 : 0; }
static uint32_t kp_fn(void) { return ui.kpage == KP_TERROR ? 3u : ui.kpage == KP_TMORE ? 4u : 1u; }
static int kp_terror(void) { return ui.kpage >= KP_TERROR && ui.kpage <= KP_TERROR2; }
/* LOAF BREAKER: the kick's SYNTH (kick.h KS_*), its value as the foot shows it, as its bar, turned */
static uint32_t ks_of(int i) { return (ui.kpage == KP_SYN2 ? 4u : 0u) + (uint32_t)(i & 3); }
/* LOAF BREAKER TEST: the kick's BASS part: K1 GLIDE (0 .. 400 ms between two notes), K2 REV BASS (its level), K3 its
 * START (1/8 .. 5/8 of the beat), K4 its CURVE (slow .. sharp) */
static void kb_value(int i, char *t)
{
    uint32_t v = set.kbass[i & 3];
    t[0] = 0;
    if (i == 0 || i == 1) {
        if (!v) {
            text_cat(t, "OFF");
            return;
        }
        int_text(i == 0 ? (int32_t)(v * v * 4u / 100u) : (int32_t)v, t);
        text_cat(t, i == 0 ? " MS" : "%");
    } else if (i == 2) {
        int_text((int32_t)(125u + v * 5u) / 10, t);
        text_cat(t, "% BEAT");
    } else
        text_cat(t, v < 25u ? "SLOW" : v < 50u ? "SOFT" : v < 75u ? "SHARP" : "SNAP");
}
static void ks_text(uint32_t j, char *t);
static void ks_value(int i, char *t)
{
    if (ui.kpage == KP_BASS) {
        kb_value(i, t);
        return;
    }
    ks_text(ks_of(i), t);
}
static void ks_text(uint32_t j, char *t)   /* (LOAF BREAKER: a SYNTH setting's value, by its KS_*: SEL + knob's too) */
{
    int32_t v = set.ksyn[j];
    t[0] = 0;
    if (j == KS_TIME || j == KS_RTIME) {
        float ms = j == KS_TIME ? 120.0f * brk_exp2f_c((float)v * 0.04f) : 2048.0f * brk_exp2f_c((float)v * 0.02f) / 44.1f;
        int_text((int32_t)(ms + 0.5f), t);
        text_cat(t, " MS");
    } else if (j == KS_HOLD && v > 0) {
        int_text(v * 5, t);
        text_cat(t, " MS");
    } else if ((j == KS_RUMBLE || j == KS_ASYM || j == KS_HOLD) && v <= 0)
        text_cat(t, j == KS_HOLD && v < 0 ? "SHORTER" : "OFF");
    else if (j == KS_RUMBLE || j == KS_ASYM) {
        int_text(v * 2, t);
        text_cat(t, "%");
    } else {
        if (v > 0)
            text_cat(t, "+");
        int_text(v, t + (v > 0 ? 1 : 0));
        if (j == KS_TUNE)
            text_cat(t, " ST");
    }
}
static float ks_normj(uint32_t j);
static float ks_norm(int i)
{
    if (ui.kpage == KP_BASS)
        return (float)set.kbass[i & 3] * 0.01f;
    return ks_normj(ks_of(i));
}
static float ks_normj(uint32_t j)
{
    float v = (float)set.ksyn[j];
    return j == KS_TUNE ? (v + 12.0f) / 24.0f : j == KS_RUMBLE || j == KS_ASYM ? v / 50.0f : (v + 50.0f) * 0.01f;
}
static void ks_add(uint32_t j, int32_t d);
static void ks_turn(int i, int32_t d)
{
    if (ui.kpage == KP_BASS) {
        set.kbass[i & 3] = (uint8_t)clamp((int32_t)set.kbass[i & 3] + d * 2, 0, 100);
        return;
    }
    ks_add(ks_of(i), d);
}
static void ks_add(uint32_t j, int32_t d)   /* (a SYNTH setting turned, by its KS_*) */
{
    int32_t v = set.ksyn[j];
    v = j == KS_TUNE ? clamp(v + (d > 0 ? 1 : -1), -12, 12) : clamp(v + d * 2, j == KS_RUMBLE || j == KS_ASYM ? 0 : -50, 50);
    set.ksyn[j] = (int8_t)v;
}

/* LOAF BREAKER: the kick memories, KICK 1..10 (KICK's PRESETS, SEQ's SELECT): the one playing kept as it is, the next
 * as it was left (the volume kept), its knobs with it: ENV KNOB's choices, CLEAN SUB, BASS, MORPH (kx_put, kx_get) */
static uint32_t kpre_mem(uint32_t slot)  /* (0.9.2, FLEXOR: the PRESET a kick memory shows: the one it came from; never
                                          * picked, the built-in it began as: settings_defaults' order) */
{
    const kick_lab_t *l = slot % KICK_MEMS == set.kslot % KICK_MEMS ? &klab.cur : klmem_at(slot % KICK_MEMS);
    return l->pre ? (uint32_t)(l->pre - 1u) : slot % KICK_PRESETS_MEM;
}
static void kick_mem_go(uint32_t n)      /* (LOAF BREAKER: KICK n, 0..9, as above; a track item's too) */
{
    float vol = set.kick.volume;
    char t[4];
    uint32_t j;
    (*kmem_at(set.kslot % KICK_MEMS)) = set.kick;
    for (j = 0; j < KS_N; j++)           /* (LOAF BREAKER: its SYNTH with it) */
        ksmem_at(set.kslot % KICK_MEMS)[j] = set.ksyn[j];
    kx_put(set.kslot);
    kfx_put(set.kslot);                  /* (0.9.26, FLEXOR: its FX section, EQ, FX1, FX2, with it) */
    (*klmem_at(set.kslot % KICK_MEMS)) = klab.cur;   /* (LOAF BREAKER: its KICK LAB with it) */
    set.kslot = (uint8_t)(n % KICK_MEMS);
    klab.cur = (*klmem_at(set.kslot));
    set.kick = (*kmem_at(set.kslot));
    for (j = 0; j < KS_N; j++)
        set.ksyn[j] = ksmem_at(set.kslot)[j];
    kx_get(set.kslot);
    kfx_get(set.kslot);
    changed_fx();                        /* (its FX section to the engine) */
    set.kick.volume = vol;
    ui.kpreset = (uint8_t)kpre_mem(set.kslot);   /* (its PRESET: its own, 0.9.2) */
    ab_take();                           /* (LOAF BREAKER: TOOLS' A/B: this kick as it comes) */
    int_text((int32_t)set.kslot + 1, t);
    app_say("KICK", t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed_kick();
}
/* LOAF BREAKER (0.9.32, FLEXOR): a built-in kick's part outside kick_t, after the SYNTH and the LAB zeroed (app.c
 * kt_extra: its SYNTH, LAB, KICK EQ; 0.9.39 every preset's but ROTTERDAM's and DISTORTO's, which leave the EQ as it was) */
static void kpre_extra(uint32_t p)
{
    if (kt_extra(p, set.ksyn, &klab.cur, set.keq))
        changed_fx();
}
/* LOAF BREAKER: OUT's K4 PRESET: the built-in kicks (KICK_PRESET_ORD's round, 0.9.39), then COPY KICK 1..20: that
 * memory's kick, its SYNTH and its knobs (KNOBS, CLEAN SUB, BASS, MORPH) onto the kick selected, its volume kept.
 * ui.kpreset: a built-in's table index (< KICK_PRESETS: what kick_lab_t.pre stores, + 1), or KICK_PRESETS + n, COPY K n */
#define KPRE_N (KICK_PRESETS_ON + KICK_MEMS)   /* (the round's length) */
static uint32_t kpre_pos(uint32_t v)     /* (ui.kpreset's place in the round; DISTORTO, out of it: INDUSTRIAL's) */
{
    uint32_t j;
    if (v >= KICK_PRESETS)
        return KICK_PRESETS_ON + (v - KICK_PRESETS) % KICK_MEMS;
    for (j = 0; j < KICK_PRESETS_ON; j++)
        if (KICK_PRESET_ORD[j] == (v == KICK_PRESET_DISTORTO ? 4u : v))
            return j;
    return 0;
}
static uint32_t kpre_at(uint32_t pos)
{
    pos %= KPRE_N;
    return pos < KICK_PRESETS_ON ? KICK_PRESET_ORD[pos] : KICK_PRESETS + pos - KICK_PRESETS_ON;
}
static void kpre_text(char *t)
{
    uint32_t p = ui.kpreset;
    if (p < KICK_PRESETS) {
        text_cat(t, KICK_PRESET_NAME[p]);
        return;
    }
    text_cat(t, "COPY K");
    int_text((int32_t)((p - KICK_PRESETS) % KICK_MEMS) + 1, t + 6);
}
static void kick_copy_from(uint32_t n)    /* (0.9.26: kick memory n's everything onto the kick playing, its volume kept: COPY
                                          * K n, SEL + ARP's paste) */
{
    float vol = set.kick.volume;
    uint32_t j;
    set.kick = (*kmem_at(n));
    set.kick.volume = vol;
    for (j = 0; j < KS_N; j++)
        set.ksyn[j] = ksmem_at(n)[j];
    kx_get(n);
    kfx_get(n);
    klab.cur = (*klmem_at(n));
    *kx2_at(set.kslot) = *kx2_at(n);     /* (0.9.45: its TOK and TAIL GATE too) */
    changed_kick();
    changed_fx();
}
static void kick_mem_turn(int32_t d)
{
    kick_mem_go((set.kslot % KICK_MEMS + (d > 0 ? 1u : KICK_MEMS - 1u)) % KICK_MEMS);
}
static const char *const KPAGE_NAME[KPAGES] = {"MIX", "LAYER", "LAYER FX", "MACRO", "OUT", "DISTO", "DISTO +",
                                                "SCREAM", "SYNTH", "SYNTH +", "BASS", "KNOBS", "K SET",
                                                "CLEAN MORPH", "ASSIGN", "TAIL", "TAIL +", "STACK", "TOOLS", "PRESET",
                                                "TOK", "TAIL GATE"};
/* the kick's parts in a row, as ALGORITHM turns through them: MIX, SUB, SUB FX, PUNCH, PUNCH FX, GRIT, GRIT FX,
 * CLICK, CLICK FX, MACRO, OUT (each a sub-page, and for a layer its layer); LOAF BREAKER TEST: then EQ BANDS, EQ
 * SHAPE (the KICK EQ page), FX 1, FX 2 (the FX page on the kick's slots), TERROR, TERROR +, TERROR 2 */
#define KP_BASE (3u + 2u * KICK_LAYERS)
#define KPARTS (KP_BASE + 24u)       /* (LOAF BREAKER: then the LAB's TAIL, TAIL +, STACK, TOOLS; then PRESET, 0.253; TOK,
                                          * TAIL GATE, 0.9.45) */
#define KP_PART_PRESET (KP_BASE + 21u)
#define KP_PART_TOK (KP_BASE + 22u)
#define KP_PART_GATE (KP_BASE + 23u)
static const char *const KP_MORE_NAME[11] = {"EQ BANDS", "EQ SHAPE", "FX 1", "FX 2", "DISTO", "DISTO +", "SCREAM",
                                            "SYNTH", "SYNTH +", "BASS", "KNOBS"};   /* (then K1 SET .. K4 SET: "K1 C SUB") */   /* (SYNTH, SYNTH +: ALGORITHM left from MIX) */
static uint32_t kick_part(void)
{
    if (ui.page == PG_KEQ)
        return KP_BASE + (ui.keq_part & 1u);
    if (ui.page == PG_KICK && ui.kpage == KP_PRESET)
        return KP_PART_PRESET;
    if (ui.page == PG_KICK && (ui.kpage == KP_TOK || ui.kpage == KP_GATE))
        return ui.kpage == KP_TOK ? KP_PART_TOK : KP_PART_GATE;
    if (ui.page == PG_FX)
        return KP_BASE + 2u + (uint32_t)(ui.fx_focus - BRK_FX_SLOTS) % 2u;
    if (ui.kpage == KP_CMORPH)
        return KP_BASE + 15u;
    if (ui.kpage == KP_STYLE)
        return KP_BASE + 16u;
    if (klab_page())
        return KP_BASE + 17u + (uint32_t)(ui.kpage - KP_TAIL);
    if (kp_knobs())
        return KP_BASE + 10u + (ui.kpage == KP_KSET ? 1u + (ui.kks & 3u) : 0u);
    if (ui.kpage >= KP_TERROR)
        return KP_BASE + 4u + (uint32_t)(ui.kpage - KP_TERROR);
    return ui.kpage == KP_MIX ? 0u : ui.kpage == KP_MACRO ? KP_BASE - 2u : ui.kpage == KP_OUT ? KP_BASE - 1u
           : 1u + ui.klayer * 2u + (ui.kpage == KP_FX ? 1u : 0u);
}
static void kick_part_name(uint32_t p, char *t)
{
    if (p == KP_PART_PRESET) {
        text_cat(t, "PRESET");
        return;
    }
    if (p == KP_PART_TOK || p == KP_PART_GATE) {
        text_cat(t, p == KP_PART_TOK ? "TOK" : "TAIL GATE");
        return;
    }
    if (p == KP_BASE + 15u) {
        text_cat(t, "CLEAN MORPH");
        return;
    }
    if (p == KP_BASE + 16u) {            /* (STYLE until 0.9.1: ASSIGN, FLEXOR) */
        text_cat(t, "ASSIGN");
        return;
    }
    if (p >= KP_BASE + 17u) {            /* (LOAF BREAKER: the LAB) */
        text_cat(t, KPAGE_NAME[KP_TAIL + (p - KP_BASE - 17u) % 4u]);
        return;
    }
    if (p >= KP_BASE + 11u) {            /* (K1 SET ..: the knob, its function) */
        char k[3] = {'K', (char)('1' + (p - KP_BASE - 11u) % 4u), 0};
        text_cat(t, k);
        text_cat(t, " ");
        text_cat(t, sk_short(selk_fn((p - KP_BASE - 11u) % 4u)));
        return;
    }
    if (p >= KP_BASE) {
        text_cat(t, KP_MORE_NAME[(p - KP_BASE) % 11u]);
        return;
    }
    if (p == 0u || p >= KP_BASE - 2u) {
        text_cat(t, KPAGE_NAME[p == 0u ? KP_MIX : p == KP_BASE - 2u ? KP_MACRO : KP_OUT]);
        return;
    }
    text_cat(t, KICK_LAYER_NAME[(p - 1u) / 2u]);
    if (!((p - 1u) & 1u))
        return;
    text_cat(t, " FX");
}
/* LOAF BREAKER (0.205): the KICK menu by the kick's anatomy, EDIT the next section, ALGORITHM the next part in it (a row
 * of them at the picture's foot, the one open lit). ATTACK: PRESET (0.253), CLICK, PUNCH, STACK. BODY: MIX, SUB, GRIT, SYNTH, SYNTH +,
 * MACRO, OUT. TAIL: TAIL, TAIL +, BASS, CLEAN MORPH. DIRT: DISTO, DISTO +, SCREAM, the layers' effects. TONE: EQ
 * BANDS, EQ SHAPE, FX 1, FX 2. KNOBS: KNOBS, K1-K4 SET, STYLE, TOOLS. The menu opens on BODY (app_ui_init) */
#define KSECTS 7u
static const char *const KSECT_NAME[KSECTS] = {"ATTACK", "BODY", "TAIL", "DIRT", "FX1", "FX2", "MORE"};   /* (0.9.29,
                                          * FLEXOR: FX1 and FX2 sections of their own, ALGORITHM there their parts; the
                                          * EQ in MORE, KNOBS until then) */   /* (FX: TONE until
                                          * 0.9.1, FLEXOR) */
static const uint8_t KSECT_N[KSECTS] = {5u, 7u, 5u, 7u, 1u, 1u, 9u};
static const uint8_t KSECT[KSECTS][9] = {{32u, 7u, 3u, 30u, 33u},   /* (ATTACK: PRESET first, 0.253; TOK last, 0.9.45:
                                          * the parts before where they were) */
                                         {0u, 1u, 5u, 18u, 19u, 9u, 10u}, {28u, 29u, 20u, 26u, 34u},   /* (TAIL: its GATE
                                          * last, 0.9.45) */
                                        {15u, 16u, 17u, 2u, 4u, 6u, 8u}, {13u}, {14u},
                                        {21u, 27u, 22u, 23u, 24u, 25u, 31u, 11u, 12u}};   /* (KNOBS: KP_BASE + 10 ..; 26 CLEAN MORPH;
                                          * 0.9.29, FLEXOR: ASSIGN right after KNOBS) */
static void ksect_find(uint32_t p)       /* the part's section and place in it, kept */
{
    uint32_t s, j;
    for (s = 0; s < KSECTS; s++)
        for (j = 0; j < KSECT_N[s]; j++)
            if (KSECT[s][j] == p) {
                ui.ksect = (uint8_t)s;
                ui.kpos[s] = (uint8_t)j;
                return;
            }
}
static void kick_part_set(uint32_t p);
/* LOAF BREAKER TEST: GLO MORE's MODE BEGINNER: ALGORITHM passes over the advanced parts (each as it is, EXPERT shows
 * it again): SOUND MIX, SUB, OUT; SYNTH SYNTH; DIRT DISTO; TONE EQ BANDS, FX 1; KNOBS KNOBS, STYLE (10 Nov 2026:
 * MACRO and BASS for EXPERT too) */
#define KP_ADV ((1u << 2) | (1u << 4) | (1u << 5) | (1u << 6) | (1u << 8) | (1u << 9) | \
                (1u << 12) | (1u << 14) | (1u << 16) | (1u << 17) | (1u << 19) | (1u << 20) | (1u << 22) | (1u << 23) | \
                (1u << 24) | (1u << 25) | (1u << 26) | (1u << 29) | (1u << 31))   /* (LAB: TAIL +, TOOLS) */
static int kp_hidden(uint32_t p) { return set.novice && p < 32u && (KP_ADV >> p & 1u); }
/* LOAF BREAKER (FLEXOR, 11 Nov 2026): BEGINNER's KICK, on OUT's part: 0.223 one page; 0.228 two; 0.229 three, FLEXOR's
 * eight picked on a checklist and MIX: KICK K1 PRESET (the built-in kicks), K2 DISTO, K3 SUB DRIVE (the SUB layer's DRIVE
 * effect, its GAIN: OFF at the left), K4 DRIVE (the kick's INTENSITY); SHAPE K1 P DROP (SYNTH's DROP), K2 DROP TIME
 * (SYNTH's TIME), K3 TAIL (the SUB's decay), K4 TAIL MODE (the LAB's: CLASSIC, CLEAN); MIX the four layers' levels, SUB,
 * PUNCH, GRIT, CLICK. EDIT again and ALGORITHM go round them (EXPERT: the menu) */
#define KP_PART_OUT 10u                  /* (OUT's part, BODY's last) */
static int kick_easy(void) { return set.novice && ui.page == PG_KICK && ui.kpage == KP_OUT; }
enum { KE_PRESET, KE_DISTO, KE_SUBDRV, KE_DRIVE, KE_DROP, KE_DTIME, KE_TAIL, KE_TMODE, KE_LSUB, KE_LPUNCH, KE_LGRIT,
       KE_LCLICK, KE_N };                /* (KE_LSUB .. KE_LCLICK: the layers' levels, in KL_* order) */
#define KE_PAGES 3u
static const uint8_t KEASY[KE_PAGES][4] = {{KE_PRESET, KE_DISTO, KE_SUBDRV, KE_DRIVE}, {KE_DROP, KE_DTIME, KE_TAIL, KE_TMODE},
                                           {KE_LSUB, KE_LPUNCH, KE_LGRIT, KE_LCLICK}};
static const char *const KE_NAME[KE_N] = {"PRESET", "DISTO", "SUB DRIVE", "DRIVE", "P DROP", "DROP TIME", "TAIL",
                                          "TAIL MODE", "SUB", "PUNCH", "GRIT", "CLICK"};
static const char *const KE_PAGE[KE_PAGES] = {"KICK", "SHAPE", "MIX"};
static uint8_t keasy_pg;                 /* (the page: 0 KICK, 1 SHAPE, 2 MIX) */
static uint32_t ke(int i) { return KEASY[keasy_pg % KE_PAGES][i & 3]; }
static void kick_open(void)              /* the KICK menu, at its section's part */
{
    uint32_t s = ui.ksect % KSECTS;
    if (set.novice) {                    /* (BEGINNER: its two pages, OUT's part; from another page, the first) */
        if (ui.page != PG_KICK)
            keasy_pg = 0u;
        kick_part_set(KP_PART_OUT);
        return;
    }
    if (kp_hidden(KSECT[s][ui.kpos[s] % KSECT_N[s]]))
        ui.kpos[s] = 0u;                 /* (BEGINNER: an advanced part left open: the section's first) */
    kick_part_set(KSECT[s][ui.kpos[s] % KSECT_N[s]]);
}
static void kick_sect_next(void)         /* EDIT on the KICK menu */
{
    if (set.novice) {                    /* (BEGINNER: KICK, SHAPE, MIX) */
        keasy_pg = (uint8_t)((keasy_pg + 1u) % KE_PAGES);
        kick_part_set(KP_PART_OUT);
        app_say("KICK", KE_PAGE[keasy_pg % KE_PAGES]);
        return;
    }
    ui.ksect = (uint8_t)((ui.ksect + 1u) % KSECTS);
    kick_open();
}
static void kick_sect_turn(int32_t d)    /* LOAF BREAKER (0.264, FLEXOR): SELECT on the KICK menu, the tabs at the top:
                                          * its sections, either way (EDIT still the next one) */
{
    ui.ksect = (uint8_t)((ui.ksect + (d > 0 ? 1u : KSECTS - 1u)) % KSECTS);
    kick_open();
}
static void kick_sect_step(int32_t d)    /* ALGORITHM on it: the tabs at the bottom, its parts */
{
    uint32_t s = ui.ksect % KSECTS, n = KSECT_N[s], k;
    if (set.novice) {                    /* (BEGINNER: KICK, SHAPE, MIX, either way; 0.264) */
        keasy_pg = (uint8_t)((keasy_pg + (d > 0 ? 1u : KE_PAGES - 1u)) % KE_PAGES);
        kick_part_set(KP_PART_OUT);
        app_say("KICK", KE_PAGE[keasy_pg % KE_PAGES]);
        return;
    }
    for (k = 0; k < n; k++) {            /* (BEGINNER: the advanced parts passed over) */
        ui.kpos[s] = (uint8_t)((ui.kpos[s] + (d > 0 ? 1u : n - 1u)) % n);
        if (!kp_hidden(KSECT[s][ui.kpos[s]]))
            break;
    }
    kick_open();
}
static void kick_part_set(uint32_t p)
{
    ksect_find(p);
    ui.page = PG_KICK;
    if (p == KP_PART_PRESET)             /* (LOAF BREAKER, 0.253: ATTACK's PRESET, the built-in kicks on K1) */
        ui.kpage = KP_PRESET;
    else if (p == KP_PART_TOK || p == KP_PART_GATE)   /* (0.9.45: ATTACK's TOK, TAIL's GATE) */
        ui.kpage = p == KP_PART_TOK ? KP_TOK : KP_GATE;
    else if (p >= KP_BASE) {                  /* (TEST) the KICK EQ, the kick's FX slots, its TERRORs */
        uint32_t q = (p - KP_BASE) % 21u;
        if (q >= 17u)                    /* (LOAF BREAKER: the LAB) */
            ui.kpage = (uint8_t)(KP_TAIL + q - 17u);
        else if (q < 2u) {
            ui.page = PG_KEQ;
            ui.keq_part = (uint8_t)q;
        } else if (q < 4u) {
            if (ui.fx_focus < BRK_FX_SLOTS)
                ui.fx_main = ui.fx_focus;
            ui.fx_focus = (uint8_t)(BRK_FX_SLOTS + q - 2u);
            ui.fx_page = 0u;
            ui.page = PG_FX;
        } else if (q == 15u)
            ui.kpage = KP_CMORPH;
        else if (q == 16u)
            ui.kpage = KP_STYLE;
        else if (q >= 10u) {             /* (KNOBS, K1 SET .. K4 SET) */
            ui.kpage = q == 10u ? KP_KNOBS : KP_KSET;
            ui.kks = (uint8_t)((q - 11u) & 3u);
        } else
            ui.kpage = (uint8_t)(KP_TERROR + q - 4u);
    } else if (p == 0u)
        ui.kpage = KP_MIX;
    else if (p == KP_BASE - 2u)
        ui.kpage = KP_MACRO;
    else if (p == KP_BASE - 1u)
        ui.kpage = KP_OUT;
    else {
        ui.kpage = (p - 1u) & 1u ? KP_FX : KP_LAYER;
        ui.klayer = (uint8_t)((p - 1u) / 2u);
    }
    page_say();
}
static const char *const KN_KICK[KPAGES][4] = {{"SUB", "PUNCH", "GRIT", "CLICK"},
                                               {"LEVEL", "PITCH", "DECAY", "TONE"},
                                               {"EFFECT", "", "", "WET"},
                                               {"SUB", "PUNCH", "GRIT", "INTENSITY"},
                                               {"VOLUME", "DISTO", "SCREAM", ""},   /* (LOAF BREAKER: OUT's K2, K3; DISTO, SCREAM: TERROR, TERROR 2 renamed) */
                                               {"AMOUNT", "SUB", "FIZZ", "SUSTAIN"},
                                               {"DRIVE", "TYPE", "SCREECH", "TAIL"},
                                               {"AMOUNT", "SUB", "SCREAM", "PUSH"},
                                               {"DROP", "TIME", "SHAPE", "HOLD"},
                                               {"RUMBLE", "LENGTH", "ASYM", "TUNE"},
                                               {"GLIDE", "REV BASS", "START", "CURVE"},
                                               {"", "", "", ""}, {"", "", "", ""},
                                               {"CLEAN SUB", "MORPH", "TO KICK", ""}, {"K1", "K2", "K3", "K4"},
                                               {"MODE", "NOTE", "CURVE", "END"}, {"FILTER", "F TIME", "DRIVE", ""},
                                               {"ATTACK", "CROSS", "FADE", "SOLO"}, {"MUTATE", "A/B", "EDIT VIEW", "DRUM ZOOM"},
                                               {"PRESET", "", "", ""},
                                               {"LEVEL", "TYPE", "PITCH", "LENGTH"}, {"DIVISION", "LENGTH", "FADE IN", ""}};   /* (KNOBS,
                                          * K SET: kick_knob_name; TOK, TAIL GATE: 0.9.45) */
/* LOAF BREAKER (0.9.45, FLEXOR's Kick Lab 2, chosen by ear): ATTACK's TOK, a short pitched attack (K1 LEVEL, K2 TYPE
 * SQUARE NOISE SINE, K3 PITCH 400 .. 4000 Hz, K4 LENGTH 2 .. 30 ms), and TAIL's GATE, the kick chopped in time from its
 * second 16th (K1 DIVISION 1/8 .. 1/32, K2 LENGTH: the part that sounds, K3 FADE IN); each kick its own (kx2) */
static const char *const TOK_TYPE[KICK_TOK_TYPES] = {"SQUARE", "NOISE", "SINE"};
static const char *const GATE_DIV[KICK_GATES] = {"OFF", "1/8", "1/12", "1/16", "1/24", "1/32"};
static int kx2_page(void) { return ui.kpage == KP_TOK || ui.kpage == KP_GATE; }
static void kx2_value(int i, char *t)
{
    const kick_x2_t *x = kx2_at(set.kslot);
    t[0] = 0;
    if (ui.kpage == KP_TOK) {
        if (i == 0) {
            if (!x->tok)
                text_cat(t, "OFF");
            else {
                int_text(x->tok, t);
                text_cat(t, "%");
            }
        } else if (i == 1)
            text_cat(t, TOK_TYPE[x->tok_type % KICK_TOK_TYPES]);
        else if (i == 2)
            hz_text(400.0f * brk_exp2f_c((float)x->tok_pitch * 0.0332193f), t);
        else {
            int_text((int32_t)(2.0f + 0.28f * (float)x->tok_len + 0.5f), t);
            text_cat(t, " MS");
        }
        return;
    }
    if (i == 0)
        text_cat(t, GATE_DIV[x->gate % KICK_GATES]);
    else if (i == 1) {
        int_text((int32_t)(10.0f + 0.8f * (float)x->gate_len + 0.5f), t);
        text_cat(t, "%");
    } else if (i == 2) {
        if (!x->gate_fade)
            text_cat(t, "OFF");
        else {
            int_text((int32_t)(20.0f + 9.8f * (float)x->gate_fade + 0.5f), t);
            text_cat(t, " MS");
        }
    }
}
static float kx2_norm(int i)
{
    const kick_x2_t *x = kx2_at(set.kslot);
    if (ui.kpage == KP_TOK)
        return i == 0 ? (float)x->tok * 0.01f : i == 1 ? (float)(x->tok_type % KICK_TOK_TYPES) * 0.5f
               : i == 2 ? (float)x->tok_pitch * 0.01f : (float)x->tok_len * 0.01f;
    return i == 0 ? (float)(x->gate % KICK_GATES) * 0.2f : i == 1 ? (float)x->gate_len * 0.01f
           : i == 2 ? (float)x->gate_fade * 0.01f : 0.0f;
}
static void kx2_turn(int i, int32_t d)
{
    kick_x2_t *x = kx2_at(set.kslot);
    int dir = d > 0 ? 1 : -1;
    char t[16];
    if (ui.kpage == KP_TOK) {
        if (i == 0) {
            if (!x->tok && dir > 0 && !x->tok_pitch && !x->tok_len) {
                x->tok_pitch = 55u;      /* (turned on the first time: HARD 909's, ~1.4 kHz, ~15 ms) */
                x->tok_len = 45u;
            }
            x->tok = (uint8_t)clamp((int32_t)x->tok + d * 2, 0, 100);
        } else if (i == 1)
            x->tok_type = (uint8_t)clamp((int32_t)(x->tok_type % KICK_TOK_TYPES) + dir, 0, (int32_t)KICK_TOK_TYPES - 1);
        else if (i == 2)
            x->tok_pitch = (uint8_t)clamp((int32_t)x->tok_pitch + d * 2, 0, 100);
        else
            x->tok_len = (uint8_t)clamp((int32_t)x->tok_len + d * 2, 0, 100);
    } else {
        if (i == 0) {
            if (!x->gate && dir > 0 && !x->gate_len)
                x->gate_len = 55u;       /* (turned on the first time: about half of each chop sounding) */
            x->gate = (uint8_t)clamp((int32_t)(x->gate % KICK_GATES) + dir, 0, (int32_t)KICK_GATES - 1);
        } else if (i == 1)
            x->gate_len = (uint8_t)clamp((int32_t)x->gate_len + d * 2, 0, 100);
        else if (i == 2)
            x->gate_fade = (uint8_t)clamp((int32_t)x->gate_fade + d * 2, 0, 100);
        else
            return;
    }
    kx2_value(i, t);
    app_say(KN_KICK[ui.kpage % KPAGES][i], t);
    changed_kick();
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* LOAF BREAKER: OUT's K2 TERROR, K3 TERROR 2 (kick.h), 0..100 each; SEL + K4 and SEL + K3 on any page (main.c) */
static uint8_t *kick_hc(kick_t *k, int i) { return i == 2 ? &KICK_TERROR2(k) : &KICK_TERROR(k); }
static void kick_hc_text(const kick_t *k, int i, char *t)
{
    uint8_t v = *kick_hc((kick_t *)k, i);
    t[0] = 0;
    if (!v) {
        text_cat(t, "OFF");
        return;
    }
    int_text(v, t);
    text_cat(t, "%");
}
static const char *const KFX_PARAM[KICK_FX][2] = {{"", ""}, {"GAIN", "TONE"}, {"BITS", "RATE"},
                                                  {"CUTOFF", "RESO"}, {"FREQ", ""}};
/* the macros: each moves a few of the sound's settings together */
#define KM_N 3u
static float *kick_macro(int m, uint32_t j)
{
    kick_t *k = &set.kick;
    switch (m) {
    case 0:                              /* SUB: its level, its tail, its weight */
        return j == 0u ? &k->l[KL_SUB].level : j == 1u ? &k->l[KL_SUB].decay : &k->l[KL_SUB].tone;
    case 1:                              /* PUNCH: its level, its sweep, the click */
        return j == 0u ? &k->l[KL_PUNCH].level : j == 1u ? &k->l[KL_PUNCH].tone : &k->l[KL_CLICK].level;
    default:                             /* GRIT: its level, its dirt, its tail */
        return j == 0u ? &k->l[KL_GRIT].level : j == 1u ? &k->l[KL_GRIT].tone : &k->l[KL_GRIT].decay;
    }
}
static float kick_macro_value(int m)
{
    uint32_t j;
    float v = 0.0f;
    for (j = 0; j < KM_N; j++)
        v += *kick_macro(m, j);
    return v * (1.0f / (float)KM_N);
}
/* LOAF BREAKER: a layer's FX page, K3 empty (no effect, or RING's one setting): PRESET, the kick back to a built-in
 * one (KICK_PRESET_ORD's round, its volume kept) */
static int kick_preset_knob(int i)
{
    return (ui.kpage == KP_PRESET && i == 0) || (ui.kpage == KP_FX && i == 2 && !KFX_PARAM[set.kick.l[ui.klayer % KICK_LAYERS].fx % KICK_FX][1][0]) ||
           (ui.kpage == KP_OUT && (kick_easy() ? ke(i) == KE_PRESET : i == 3));   /* (LOAF BREAKER: OUT's K4;
                                          * BEGINNER: KICK's K1) */
}
/* LOAF BREAKER (0.9.71, FLEXOR): TAIL MODE CLASSIC, CLEAN, SPLIT (the body dirty, the tail a clean sine), DIRTY (the
 * dirt rising along the tail); SPLIT's and DIRTY's own setting on TAIL + K4 (kick_lab_t.amt), half way at first */
static const char *const KTM_NAME[KTM_N] = {"CLASSIC", "CLEAN", "SPLIT", "DIRTY"};
static void ktm_turn(kick_lab_t *l, int32_t d)
{
    uint32_t m = (uint32_t)clamp((int32_t)(l->mode % KTM_N) + (d > 0 ? 1 : -1), 0, (int32_t)KTM_N - 1);
    if ((m == KTM_SPLIT || m == KTM_DIRTY) && l->mode != m)
        l->amt = 50u;
    l->mode = (uint8_t)m;
}
static const char *kick_knob_name(int i)
{
    if (kick_preset_knob(i))
        return "PRESET";
    if (kick_easy())                     /* (BEGINNER's pages) */
        return KE_NAME[ke(i)];
    if (ui.kpage == KP_KNOBS)
        return sk_short(kn_fn(i));
    if (ui.kpage == KP_KSET)
        return sk_kname(kn_fn(i), i);
    if (ui.kpage == KP_FX && (i == 1 || i == 2))
        return KFX_PARAM[set.kick.l[ui.klayer % KICK_LAYERS].fx % KICK_FX][i - 1];
    if (ui.kpage == KP_TAIL2 && i == 3)   /* (0.9.71: TAIL MODE's own setting, SPLIT's or DIRTY's) */
        return klab.cur.mode == KTM_SPLIT ? "SPLIT AT" : klab.cur.mode == KTM_DIRTY ? "DIRT RISE" : "";
    return KN_KICK[ui.kpage % KPAGES][i];
}
static void kick_value(int i, char *t)
{
    const kick_t *k = &set.kick;
    const kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    t[0] = 0;
    if (kx2_page()) {                    /* (0.9.45: TOK, TAIL GATE) */
        kx2_value(i, t);
        return;
    }
    if (ui.kpage == KP_PRESET) {         /* (LOAF BREAKER, 0.253: ATTACK's PRESET: K1 the built-in kick, alone) */
        if (i == 0)
            kpre_text(t);
        return;
    }
    if (kick_easy() && ke(i) != KE_PRESET) {   /* (BEGINNER's pages) */
        uint32_t e = ke(i);
        switch (e) {
        case KE_DISTO:
            kick_hc_text(k, 1, t);
            break;
        case KE_SUBDRV:
            if (k->l[KL_SUB].fx != KFX_DRIVE)
                text_cat(t, "OFF");
            else
                pct_text(k->l[KL_SUB].fa, t);
            break;
        case KE_DRIVE:
            pct_text(k->intensity, t);
            break;
        case KE_DROP:
            ks_text(KS_DROP, t);
            break;
        case KE_DTIME:
            ks_text(KS_TIME, t);
            break;
        case KE_TAIL:
            int_text((int32_t)(kick_decay_ms(KL_SUB, k->l[KL_SUB].decay) + 0.5f), t);
            text_cat(t, " MS");
            break;
        case KE_TMODE:
            text_cat(t, KTM_NAME[klab.cur.mode % KTM_N]);
            break;
        default:
            pct_text(k->l[(e - KE_LSUB) % KICK_LAYERS].level, t);
            break;
        }
        return;
    }
    if (klab_page()) {                   /* LOAF BREAKER: the LAB */
        klab_value(i, t);
        return;
    }
    if (kp_terror()) {                   /* the TERRORs */
        selk_value(kp_fn(), i, t);
        return;
    }
    if (kp_knobs()) {                    /* KNOBS, K SET */
        if (kick_knob_name(i)[0])
            selk_value(kn_fn(i), kn_i(i), t);
        return;
    }
    if (ui.kpage == KP_CMORPH) {         /* CLEAN MORPH */
        if (i < 3)
            selk_value(cm_fn(i), cm_i(i), t);
        return;
    }
    if (ui.kpage == KP_STYLE) {          /* STYLE: each knob's function */
        text_cat(t, sk_short(selk_fn((uint32_t)i)));
        return;
    }
    if (ui.kpage >= KP_SYN1 && ui.kpage <= KP_BASS) {   /* SYNTH, SYNTH + (LOAF BREAKER: and BASS) */
        ks_value(i, t);
        return;
    }
    switch (ui.kpage) {
    case KP_MIX:
        pct_text(k->l[i].level, t);
        break;
    case KP_LAYER:
        if (i == 1)
            hz_text(kick_pitch_hz(ui.klayer, l->pitch), t);
        else if (i == 2) {
            int_text((int32_t)(kick_decay_ms(ui.klayer, l->decay) + 0.5f), t);
            text_cat(t, " MS");
        } else
            pct_text(i == 0 ? l->level : l->tone, t);
        break;
    case KP_FX:
        if (kick_preset_knob(i))
            kpre_text(t);
        else if (i == 0)
            text_cat(t, KICK_FX_NAME[l->fx % KICK_FX]);
        else if (i == 3)
            pct_text(l->wet, t);
        else if (!kick_knob_name(i)[0])
            ;
        else if (l->fx == KFX_CRUNCH && i == 1) {
            int_text((int32_t)kick_fx_bits(l->fa), t);
            text_cat(t, " BIT");
        } else if ((l->fx == KFX_FILTER || l->fx == KFX_RING) && i == 1)
            hz_text(kick_fx_hz(l->fx, l->fa), t);
        else
            pct_text(i == 1 ? l->fa : l->fb, t);
        break;
    case KP_MACRO:
        pct_text(i == 3 ? k->intensity : kick_macro_value(i), t);
        break;
    default:
        if (kick_preset_knob(i))         /* (LOAF BREAKER: OUT's K4: the built-in kick) */
            kpre_text(t);
        else if (i == 0)
            pct_text(k->volume, t);
        else if (i <= 2)                 /* LOAF BREAKER: TERROR, TERROR 2 */
            kick_hc_text(k, i, t);
        break;
    }
}
/* LOAF BREAKER: a kick layer's effect remembered as it was left (pats.mem), each layer's each effect */
static void kfx_mem_put(uint32_t layer, const kick_layer_t *l)
{
    uint16_t *m;
    if (!l->fx || l->fx >= KICK_FX)
        return;
    m = pats.mem.kfx[layer % KICK_LAYERS][l->fx - 1u];
    m[0] = fxm_q(l->fa);
    m[1] = fxm_q(l->fb);
    m[2] = fxm_q(l->wet);
    pats.mem.kfx_ok |= (uint16_t)(1u << ((layer % KICK_LAYERS) * 4u + l->fx - 1u));
}
static int kfx_mem_get(uint32_t layer, kick_layer_t *l)
{
    const uint16_t *m;
    if (!l->fx || l->fx >= KICK_FX || !(pats.mem.kfx_ok >> ((layer % KICK_LAYERS) * 4u + l->fx - 1u) & 1u))
        return 0;
    m = pats.mem.kfx[layer % KICK_LAYERS][l->fx - 1u];
    l->fa = fxm_f(m[0]);
    l->fb = fxm_f(m[1]);
    l->wet = fxm_f(m[2]);
    return 1;
}
/* SEL clicked twice on EDIT KICK's FX part: the layer's effect back to its start (twice again within 4 s: undone) */
static struct { float fa, fb, wet; uint32_t t; uint8_t layer, fx; } kfx_undo;
static void kfx_reset(void)
{
    uint32_t ly = ui.klayer % KICK_LAYERS, now = plat_ms();
    kick_layer_t *l = &set.kick.l[ly];
    if (kfx_undo.t && now - kfx_undo.t < 4000u && kfx_undo.layer == ly && kfx_undo.fx == l->fx) {
        l->fa = kfx_undo.fa;
        l->fb = kfx_undo.fb;
        l->wet = kfx_undo.wet;
        kfx_undo.t = 0u;
        app_say(KICK_FX_NAME[l->fx % KICK_FX], "RESET UNDONE");
    } else {
        if (!l->fx) {
            app_say("KICK FX", "OFF");
            return;
        }
        kfx_undo.fa = l->fa;
        kfx_undo.fb = l->fb;
        kfx_undo.wet = l->wet;
        kfx_undo.layer = (uint8_t)ly;
        kfx_undo.fx = l->fx;
        kfx_undo.t = now | 1u;
        l->fa = l->fb = 0.5f;
        l->wet = 1.0f;
        pats.mem.kfx_ok &= (uint16_t)~(1u << (ly * 4u + l->fx - 1u));
        app_say(KICK_FX_NAME[l->fx % KICK_FX], "RESET (SEL+REC: UNDO)");
    }
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed_kick();
}
static void kick_knob_turn(int i, int32_t d)
{
    kick_t *k = &set.kick;
    kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    char t[16];
    if (ui.kpage == KP_PRESET && i != 0)
        return;                          /* (ATTACK's PRESET: K1 alone) */
    if (kx2_page()) {                    /* (0.9.45: TOK, TAIL GATE) */
        kx2_turn(i, d);
        return;
    }
    if (kick_easy() && ke(i) != KE_PRESET) {   /* (BEGINNER's pages) */
        char w[24];
        kick_layer_t *sub = &k->l[KL_SUB];
        uint32_t e = ke(i);
        switch (e) {
        case KE_DISTO:                   /* (OUT's K2: 2 % a detent) */
            KICK_TERROR(k) = (uint8_t)clamp((int32_t)KICK_TERROR(k) + d * 2, 0, 100);
            break;
        case KE_SUBDRV:                  /* (left to nothing: OFF; from OFF, right: on, from its least) */
            if (sub->fx != KFX_DRIVE) {
                if (d <= 0)
                    break;
                sub->fx = KFX_DRIVE;
                sub->fa = 0.0f;
                if (sub->wet < 0.05f)
                    sub->wet = 1.0f;
            }
            sub->fa = knob_f(sub->fa, d, 0.02f, 0.0f, 1.0f);
            if (d < 0 && sub->fa <= 0.0f)
                sub->fx = KFX_OFF;
            break;
        case KE_DRIVE:
            k->intensity = knob_f(k->intensity, d, 0.02f, 0.0f, 1.0f);
            break;
        case KE_DROP:
            ks_add(KS_DROP, d);
            break;
        case KE_DTIME:
            ks_add(KS_TIME, d);
            break;
        case KE_TAIL:
            sub->decay = knob_f(sub->decay, d, 0.02f, 0.0f, 1.0f);
            break;
        case KE_TMODE:                   /* (the LAB's TAIL MODE: CLASSIC, CLEAN, SPLIT, DIRTY, 0.9.71) */
            ktm_turn(&klab.cur, d);
            break;
        default: {
            kick_layer_t *l = &k->l[(e - KE_LSUB) % KICK_LAYERS];
            l->level = knob_f(l->level, d, 0.02f, 0.0f, 1.0f);
            break;
        }
        }
        kick_value(i, t);
        w[0] = 0;
        text_cat(w, "KICK ");
        text_cat(w, e == KE_DROP ? "PITCH DROP" : KE_NAME[e]);
        app_say(w, t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    }
    if (klab_page()) {                   /* LOAF BREAKER: the LAB */
        klab_turn(i, d);
        return;
    }
    if (kp_terror()) {                   /* the TERRORs */
        selk_turn(kp_fn(), i, d);
        return;
    }
    if (kp_knobs()) {                    /* KNOBS, K SET */
        if (kick_knob_name(i)[0])
            selk_turn(kn_fn(i), kn_i(i), d);
        return;
    }
    if (ui.kpage == KP_CMORPH) {         /* CLEAN MORPH */
        if (i < 3)
            selk_turn(cm_fn(i), cm_i(i), d);
        return;
    }
    if (ui.kpage == KP_STYLE) {          /* STYLE: K1-K4 each its own knob's function (PRESETS: a style) */
        selk_assign_k((uint32_t)i, d);
        return;
    }
    if (ui.kpage >= KP_SYN1 && ui.kpage <= KP_BASS) {   /* SYNTH, SYNTH + (LOAF BREAKER: and BASS) */
        char w[24];
        ks_turn(i, d);
        ks_value(i, t);
        w[0] = 0;
        text_cat(w, "KICK ");
        text_cat(w, KN_KICK[ui.kpage % KPAGES][i & 3]);
        app_say(w, t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    }
    if (kick_preset_knob(i)) {           /* the whole kick: every layer, its effects, INTENSITY (the kick selected,
                                          * KICK 1-10: its memory takes it); the volume and the steps kept */
        uint32_t pos = kpre_pos(ui.kpreset);
        if (ui.kpreset == KICK_PRESET_DISTORTO && d < 0)   /* (a kick saved from DISTORTO: back to INDUSTRIAL) */
            pos++;
        ui.kpreset = (uint8_t)kpre_at(pos + (d > 0 ? 1u : KPRE_N - 1u));
        if (ui.kpreset < KICK_PRESETS) {
            uint32_t j;
            kick_preset(k, ui.kpreset);
            tut_kpre++;                  /* (the TUTORIAL's count) */
            for (j = 0; j < KS_N; j++)   /* (LOAF BREAKER: its SYNTH back to as it always was too) */
                set.ksyn[j] = 0;
            memset(&klab.cur, 0, sizeof klab.cur);   /* (and its KICK LAB off) */
            memset(kx2_at(set.kslot), 0, sizeof(kick_x2_t));   /* (0.9.45: its TOK and TAIL GATE off, as the LAB) */
            klab.cur.pre = (uint8_t)(ui.kpreset + 1u);   /* (the PRESET it shows, as its memory's, 0.9.2) */
            kpre_extra(ui.kpreset);
            if (KICK_TERROR(k))          /* (0.9.40, FLEXOR: a preset made with its DISTO sounds with it: the DST
                                          * switch on again; its SUB DRIVE, the SUB layer's effect, is the preset's) */
                set.selk_hi &= (uint8_t)~16u;
            if (k->intensity > 0.005f)   /* (0.9.43: and with its DRIVE: DRV on again) */
                set.selk_hi &= (uint8_t)~32u;
            if (k->l[KL_SUB].fx != KFX_OFF)   /* (0.9.43: and with its SUB FX: SUB on again) */
                set.selk_hi &= (uint8_t)~64u;
        } else {                         /* (COPY KICK n: that memory's, if it is another) */
            uint32_t n = (ui.kpreset - KICK_PRESETS) % KICK_MEMS, j;
            float vol = k->volume;
            kick_value(i, t);
            if (n == set.kslot % KICK_MEMS) {
                app_say(t, "(THIS ONE)");
                ui.redraw |= RG_FOOT;
                return;
            }
            *k = (*kmem_at(n));
            k->volume = vol;
            for (j = 0; j < KS_N; j++)
                set.ksyn[j] = ksmem_at(n)[j];
            kx_get(n);
            kfx_get(n);                  /* (0.9.26: its FX section too) */
            changed_fx();
            klab.cur = (*klmem_at(n));      /* (LOAF BREAKER: its KICK LAB too) */
            app_say(t, "ON THIS KICK");
            ui.redraw |= RG_BLOCKS | RG_FOOT;
            changed_kick();
            return;
        }
        kick_value(i, t);
        app_say("KICK PRESET", t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    }
    switch (ui.kpage) {
    case KP_MIX:
        k->l[i].level = knob_f(k->l[i].level, d, 0.02f, 0.0f, 1.0f);
        break;
    case KP_LAYER: {
        float *p = i == 0 ? &l->level : i == 1 ? &l->pitch : i == 2 ? &l->decay : &l->tone;
        *p = knob_f(*p, d, i == 1 ? 0.01f : 0.02f, 0.0f, 1.0f);
        break;
    }
    case KP_FX:
        if (i == 0) {                    /* (LOAF BREAKER: the effect left remembered, the new one as left) */
            uint32_t f = (uint32_t)clamp((int32_t)l->fx + (d > 0 ? 1 : -1), 0, KICK_FX - 1);
            if (f == l->fx)
                break;
            kfx_mem_put(ui.klayer % KICK_LAYERS, l);
            l->fx = (uint8_t)f;
            if (!kfx_mem_get(ui.klayer % KICK_LAYERS, l)) {
                l->fa = l->fb = 0.5f;    /* (kick_preset's) */
                l->wet = 1.0f;
            }
        } else if (i == 3)
            l->wet = knob_f(l->wet, d, 0.02f, 0.0f, 1.0f);
        else if (kick_knob_name(i)[0]) {
            float *p = i == 1 ? &l->fa : &l->fb;
            *p = knob_f(*p, d, 0.02f, 0.0f, 1.0f);
        }
        break;
    case KP_MACRO:
        if (i == 3)
            k->intensity = knob_f(k->intensity, d, 0.02f, 0.0f, 1.0f);
        else {
            uint32_t j;
            for (j = 0; j < KM_N; j++)
                *kick_macro(i, j) = knob_f(*kick_macro(i, j), d, 0.02f, 0.0f, 1.0f);
        }
        break;
    default:
        if (i == 0)
            k->volume = knob_f(k->volume, d, 0.02f, 0.0f, 1.0f);
        else if (i <= 2) {               /* LOAF BREAKER: TERROR, TERROR 2, 2 % a detent */
            uint8_t *p = kick_hc(k, i);
            *p = (uint8_t)clamp((int32_t)*p + d * 2, 0, 100);
        } else
            return;
        break;
    }
    kick_value(i, t);
    if (ui.kpage == KP_LAYER || (ui.kpage == KP_FX && i)) {   /* "GRIT  DECAY 240 MS" */
        char w[24];
        w[0] = 0;
        text_cat(w, kick_knob_name(i));
        text_cat(w, " ");
        text_cat(w, t);
        app_say(KICK_LAYER_NAME[ui.klayer % KICK_LAYERS], w);
    } else
        app_say(kick_knob_name(i), t);
    ui.redraw |= RG_BLOCKS;
    changed_kick();
}

/* the KICK page's strip: the kick's shape and its layers' levels; then its 16 steps (the one playing outlined),
 * what the page edits and where the kick goes */

/* EDIT's lower strip: the drums side by side (LOAF BREAKER: the clap too), each its own waveform (one hit, worked out
 * again when its sound changes), the one edited outlined, lit on its hits */
#define DW 36u                           /* a drum's waveform: its columns */
static uint8_t drum_px[NDRUMS][DW];
static uint32_t drum_sig[NDRUMS] = {1u, 1u, 1u, 1u};
static uint32_t sig_of(const void *p, uint32_t n)
{
    uint32_t i, h = 2166136261u;
    for (i = 0; i < n; i++)
        h = (h ^ ((const uint8_t *)p)[i]) * 16777619u;
    return h;
}
static void draw_drums(const brk_status_t *st)
{
    uint32_t i, c;
    for (i = 0; i < NDRUMS; i++) {
        uint32_t sig = i ? sig_of(perc_voice(i - 1u), sizeof(perc_voice_t))
                         : sig_of(&set.kick, sizeof set.kick) ^ sig_of(set.ksyn, sizeof set.ksyn);
        if (sig != drum_sig[i]) {        /* its hit again, over a window that fits its decay, as tall as the box */
            uint32_t pk = 0u;
            drum_sig[i] = sig;
            if (i) {
                float d = perc_decay_ms(i - 1u, perc_voice(i - 1u)->decay) * 1.4f;
                perc_shape(i - 1u, perc_voice(i - 1u), drum_px[i], DW, (uint32_t)brk_clampf(d, 60.0f, 900.0f), 120u);
            } else
                kick_shape(&set.kick, drum_px[0], DW, 600u, 120u);
            for (c = 0; c < DW; c++)
                pk = drum_px[i][c] > pk ? drum_px[i][c] : pk;
            for (c = 0; c < DW && pk; c++)
                drum_px[i][c] = (uint8_t)(drum_px[i][c] * 30u / pk);
        }
    }
    cv_begin(240, 52, C_BLACK);
    for (i = 0; i < NDRUMS; i++) {
        int32_t x = 1 + (int32_t)i * 48;
        int sel = i == ui.einst, hit = (st->playing && st->bar64 >= 0 &&
            (drum_now[i][st->bar64 >> 4 & 3] >> (st->bar64 & 15) & 1u)) ||
            (ui.aud_d == i && plat_ms() - ui.aud_t < 150u);   /* (LOAF BREAKER: or just played on its key) */
        char kn[2] = {(char)('1' + i), 0};
        uint16_t ink = hit ? C_BLACK : sel ? C_HI : C_GRAY;
        cv_rect(x, 2, 46, 48, sel ? C_WHITE : C_LINE);
        cv_rect(x + 1, 3, 44, 46, hit ? C_BLOCK_HI : C_BLACK);
        cv_text(x + 5, 4, &FONT_T, DRUM_NAME[i], hit ? C_BLACK : sel ? C_HI : C_GRAY);
        if (!st->playing)                /* (its key, to play it: stopped) */
            cv_text(x + 42 - text_w(&FONT_T, kn), 37, &FONT_T, kn, hit ? C_BLACK : C_DIM);
        cv_rect(x + 5, 33, DW, 1, hit ? C_BLACK : C_LINE);   /* the centre line, the hit mirrored about it */
        for (c = 0; c < DW; c++)
            if (drum_px[i][c])
                cv_rect(x + 5 + (int32_t)c, 33 - drum_px[i][c] / 2, 1, drum_px[i][c] | 1, ink);
    }
    cv_blit(0, 140);
}

/* the kick's page (EDIT): as the snare's and the hat's: its part's four settings as bars (lit), and under them the
 * four layers' levels (the one the part edits lit, amber with an effect on); then the three drums */
static float kick_knob_f(int i)          /* the part's knob i, 0..1 (as its bar) */
{
    const kick_t *k = &set.kick;
    const kick_layer_t *l = &k->l[ui.klayer % KICK_LAYERS];
    if (kx2_page())                      /* (0.9.45: TOK, TAIL GATE) */
        return kx2_norm(i);
    if (kick_easy() && ke(i) != KE_PRESET) {   /* (BEGINNER's pages) */
        uint32_t e = ke(i);
        switch (e) {
        case KE_DISTO:
            return (float)KICK_TERROR(k) * 0.01f;
        case KE_SUBDRV:
            return k->l[KL_SUB].fx == KFX_DRIVE ? k->l[KL_SUB].fa : 0.0f;
        case KE_DRIVE:
            return k->intensity;
        case KE_DROP:
            return ks_normj(KS_DROP);
        case KE_DTIME:
            return ks_normj(KS_TIME);
        case KE_TAIL:
            return k->l[KL_SUB].decay;
        case KE_TMODE:
            return (float)(klab.cur.mode % KTM_N) * (1.0f / (float)(KTM_N - 1u));
        default:
            return k->l[(e - KE_LSUB) % KICK_LAYERS].level;
        }
    }
    switch (ui.kpage) {
    case KP_MIX:
        return k->l[i].level;
    case KP_LAYER:
        return i == 0 ? l->level : i == 1 ? l->pitch : i == 2 ? l->decay : l->tone;
    case KP_FX:
        return i == 0 ? (float)l->fx / (float)(KICK_FX - 1) : i == 1 ? l->fa : i == 2 ? l->fb : l->wet;
    case KP_MACRO:
        return i == 3 ? k->intensity : kick_macro_value(i);
    case KP_TERROR:
    case KP_TMORE:
    case KP_TERROR2:
        return sk_norm(kp_fn(), i);
    case KP_SYN1:
    case KP_SYN2:
    case KP_BASS:
        return ks_norm(i);
    case KP_KNOBS:
    case KP_KSET:
        return sk_norm(kn_fn(i), kn_i(i));
    case KP_CMORPH:
        return i < 3 ? sk_norm(cm_fn(i), cm_i(i)) : 0.0f;
    case KP_STYLE:                       /* (STYLE: how far each knob's function is turned, SEL + it live) */
        return sk_norm(selk_fn((uint32_t)i), 0);
    case KP_TAIL:
    case KP_TAIL2:
    case KP_STACK:
    case KP_TOOLS:
        return klab_norm(i);
    default:
        return i == 0 ? k->volume : i <= 2 ? (float)*kick_hc((kick_t *)k, i) * 0.01f :
               (float)kpre_pos(ui.kpreset) / (float)(KPRE_N - 1u);   /* (OUT K4: the preset, the copies) */
    }
}
/* LOAF BREAKER: the KICK menu's picture (y 44..143, no waveform of the break above it): the kick as it sounds, its
 * hit over 250 ms drawn large (its wave over its level), the SUB's pitch falling across it (amber: from where its DROP starts to its
 * note), its memory, its note and the layers' levels; under it (y 144..191) the part's four knobs as cards, their
 * values large */
#define KV_W 232u
/* (the picture's time: a quarter of a second; LOAF BREAKER: the RGB view half a second, the tail's fall in it; GLO
 * EFFECTS K4 KICK VIEW on PART: CLICK's 50 ms, PUNCH's and STACK's 150, the TAIL section's whole second) */
static uint32_t kv_ms(void)
{
    if (ez_idx())                        /* (LOAF BREAKER: EDIT ZOOM, SYSTEM › EFFECTS: the time chosen, every part) */
        return EZ_MS[ez_idx()];
    if (set.kfocus) {
        if ((ui.kpage == KP_LAYER || ui.kpage == KP_FX) && ui.klayer == KL_CLICK)
            return 50u;
        if (((ui.kpage == KP_LAYER || ui.kpage == KP_FX) && ui.klayer == KL_PUNCH) || ui.kpage == KP_STACK)
            return 150u;
        if (ui.kpage == KP_TAIL || ui.kpage == KP_TAIL2 || ui.kpage == KP_BASS || ui.kpage == KP_CMORPH)
            return 1000u;
    }
    return ui.kview == 0u ? 500u : 250u;
}
#define KV_MS kv_ms()
/* the kick the picture draws: the one playing, or (KICK VIEW PART) on a layer's page that layer alone, on STACK its
 * attack's kick */
static void kv_kick(kick_t *k)
{
    uint32_t i;
    *k = set.kick;
    if (!set.kfocus)
        return;
    if (ui.kpage == KP_LAYER || ui.kpage == KP_FX) {
        for (i = 0; i < KICK_LAYERS; i++)
            if (i != ui.klayer % KICK_LAYERS)
                k->l[i].level = 0.0f;
    } else if (ui.kpage == KP_STACK && klab.cur.src)
        *k = (*kmem_at((klab.cur.src - 1u) % KICK_MEMS));
}
static uint32_t kv_part_sig(void)        /* (what the picture shows, for its redraw) */
{
    return (set.kfocus ? (uint32_t)ui.kpage * 8u + ui.klayer + 1u + (uint32_t)klab.cur.src * 99991u : 0u) + kv_ms() * 64u;
}
static uint8_t kv_px[KV_W];              /* the hit's level, column by column */
static int8_t kv_wave[KV_W];             /* and its wave (the pitch falling: the cycles stretching) */
static uint8_t kv_rgb[KV_W][4];          /* LOAF BREAKER: or its lows, mids, highs and peak (the RGB view) */
static uint32_t kv_sig;
static uint32_t kv_t;                    /* (TEST: when the hit was last worked out) */
static uint8_t kv_pend;                  /* (TEST: a change not drawn yet) */
static void kick_chip(uint32_t p, char *t)   /* (LOAF BREAKER: a part's name at the picture's foot, short) */
{
    if (p >= KP_BASE + 11u && p <= KP_BASE + 14u) {   /* (K1 SET ..: "K1") */
        t[0] = 'K';
        t[1] = (char)('1' + (p - KP_BASE - 11u) % 4u);
        t[2] = 0;
        return;
    }
    if (p == KP_BASE + 15u) {
        text_cat(t, "MORPH");
        return;
    }
    {                                    /* (the long ones short: "SYNTH +" SYN+, "DISTO +" D+, EQ's BANDS, SHAPE;
                                          * a layer's effect S FX .. C FX, below) */
        static const uint8_t P[8] = {KP_BASE + 8u, KP_BASE + 5u, KP_BASE + 0u, KP_BASE + 1u, KP_BASE + 18u,
                                     KP_BASE + 2u, KP_BASE + 3u, KP_PART_GATE};
        static const char *const S[8] = {"SYN+", "D+", "EQ B", "EQ S", "TAIL+", "FX1", "FX2", "GATE"};   /* (FX's: 0.9.2;
                                          * TAIL GATE's, 0.9.45) */
        uint32_t j;
        for (j = 0; j < 8u; j++)
            if (p == P[j]) {
                text_cat(t, S[j]);
                return;
            }
    }
    if (p >= 1u && p < KP_BASE - 2u && ((p - 1u) & 1u)) {   /* (a layer's effect: "S FX" .. "C FX") */
        char l[2] = {KICK_LAYER_NAME[(p - 1u) / 2u][0], 0};
        text_cat(t, l);
        text_cat(t, " FX");
        return;
    }
    kick_part_name(p, t);
}
/* LOAF BREAKER (0.205; 0.207 a bar; 0.9.2 its own, for the FX section's EQ and FX pages too): the section's parts in a
 * row at y, as the tabs: the whole width, the one open lit (BEGINNER: those it passes over dim); the tabs' letters if
 * they fit, else smaller. The section: the open part's */
static void ksect_row(int32_t y)
{
    uint32_t open = kick_part(), s = ui.ksect % KSECTS, n, j, w = 0u, extra;
    const felucca_font_t *f = &FONT_T;
    int32_t cx = 4;
    char c[16];
    for (j = 0; j < KSECTS * 9u; j++)    /* (the section holding the open part: the EQ and FX pages' own, FX) */
        if (j % 9u < KSECT_N[j / 9u] && KSECT[j / 9u][j % 9u] == open) {
            s = j / 9u;
            break;
        }
    n = KSECT_N[s];
    for (j = 0; j < n; j++) {
        c[0] = 0;
        kick_chip(KSECT[s][j], c);
        w += (uint32_t)text_w(&FONT_T, c) + 8u;
    }
    w += 2u * (n - 1u);
    uint32_t pad = 8u;
    if (w > KV_W) {                  /* (too wide: the small letters; 0.9.29, MORE's nine: narrower margins too) */
        f = &FONT_XS;
        for (pad = 6u; pad >= 2u; pad -= 2u) {
            for (w = 2u * (n - 1u), j = 0; j < n; j++) {
                c[0] = 0;
                kick_chip(KSECT[s][j], c);
                w += (uint32_t)text_w(&FONT_XS, c) + pad;
            }
            if (w <= KV_W)
                break;
        }
    }
    extra = w < KV_W ? KV_W - w : 0u;
    for (j = 0; j < n; j++) {
        uint32_t p = KSECT[s][j];
        int32_t tw, sw;
        c[0] = 0;
        kick_chip(p, c);
        tw = text_w(f, c);
        sw = tw + (int32_t)pad + (int32_t)(extra / n) + (j < extra % n ? 1 : 0);   /* (the room shared) */
        cv_rect(cx, y, sw, 15, p == open ? C_HI : C_BLOCK);
        cv_text(cx + (sw - tw + 1) / 2, f == &FONT_T ? y + 2 : y, f, c,   /* (0.9.30, FLEXOR: the small
                                          * letters centred, they sat low and left) */
                p == open ? C_BLACK : kp_hidden(p) ? C_DIM : C_GRAY);
        cx += sw + 2;
    }
}
static void kick_picture(int32_t top)
{
    const kick_t *k = &set.kick;
    uint32_t i;
    int32_t x, py = 0;
    float hz = (klab.cur.note ? kick_lab_ms(KLM_NOTE, klab.cur.note) : kick_pitch_hz(KL_SUB, k->l[KL_SUB].pitch)) *
               brk_exp2f_c((float)set.ksyn[KS_TUNE] * (1.0f / 12.0f));   /* (LOAF BREAKER: LAB's NOTE) */
    float drop = 2.0f * brk_exp2f_c((float)set.ksyn[KS_DROP] * 0.04f);
    float tms = 120.0f * brk_exp2f_c((float)set.ksyn[KS_TIME] * 0.04f);
    char t[32], u[8];                    /* (t: the part's words, "PRESETS: EARLY NEXT GEN": room to spare) */
    cv_oy = -top;
    cv_rect(4, 45, KV_W, 1, C_LINE);     /* (the middle; LOAF BREAKER: up, the parts' bar under it) */
    if (ui.kview == 0u) {                /* LOAF BREAKER: the RGB view, as rekordbox (the CDJ view's colours): each
                                          * column as tall as its peak, its lows red, mids green, highs blue */
        uint32_t pk = 1u;
        for (i = 0; i < KV_W; i++)
            pk = kv_rgb[i][3] > pk ? kv_rgb[i][3] : pk;
        for (i = 0; i < KV_W; i++) {
            const uint8_t *v = kv_rgb[i];
            uint32_t m = v[0] > v[1] ? v[0] : v[1], h = (uint32_t)v[3] * 29u / pk, br;
            m = v[2] > m ? v[2] : m;
            if (!m || !h)
                continue;
            br = 120u + (uint32_t)v[3] * 135u / pk;
            /* (each band against the strongest, squared in RGB: the colour saturated, a distorted kick orange, a
             * clean one red, a screech green-white; WAVE TYPE's other styles, 0.9.70) */
            wave_col(4 + (int32_t)i, 45, (int32_t)h, v[0], v[1], v[2], br, ws_style(), 1);
        }
    } else
    for (i = 0; i < KV_W; i++) {         /* the hit: its level (dim, mirrored), its wave over it */
        int32_t h = kv_px[i], w = kv_wave[i];
        if (h)
            cv_rect(4 + (int32_t)i, 45 - h, 1, 2 * h + 1, C_BLOCK);
        if (w > 0)
            cv_rect(4 + (int32_t)i, 45 - w, 1, w, C_HI);
        else if (w < 0)
            cv_rect(4 + (int32_t)i, 46, 1, -w, C_HI);
    }
    if (kscope_hit && !(set.kfocus && (ui.kpage == KP_LAYER || ui.kpage == KP_FX || ui.kpage == KP_STACK))) {
                                          /* LOAF BREAKER TEST: the kick as it really sounds, its last hit: white over it
                                           * (not over a layer alone, KICK VIEW PART) */
        int32_t pk = 1, v;
        for (i = 0; i < KV_W && i < KSC_W; i++) {
            v = kscope[i];
            pk = (v < 0 ? -v : v) > pk ? (v < 0 ? -v : v) : pk;
        }
        for (i = 0; i < KV_W && i < KSC_W; i++) {
            v = kscope[i] * 29 / pk;
            if (v) {
                uint32_t sx = i * 250u / KV_MS;   /* (its 250 ms, on the picture's time) */
                if (sx < KV_W)
                    cv_rect(4 + (int32_t)sx, 45 - v, 1, 1, C_WHITE);
            }
        }
    }
    for (x = 0; x < (int32_t)KV_W; x += 2) {   /* the SUB's pitch: 30 Hz at the bottom .. 1 kHz at the top, as it falls */
        float ms = (float)x * ((float)KV_MS / (float)KV_W), pe = brk_exp2f_c(-6.644f * ms / tms);
        float pl = brk_clampf(1.0f - ms / (tms * 0.5f), 0.0f, 1.0f), f;
        if (klab.cur.curve == 1u)        /* (LOAF BREAKER: LAB's CURVE: LINEAR, SNAP) */
            pe = pl;
        else if (klab.cur.curve == 2u)
            pe = 1.0f - (1.0f - pl) * (1.0f - pl) * (1.0f - pl);
        f = hz * (1.0f + drop * pe);
        int32_t y = 74 - (int32_t)(brk_clampf(brk_log2f(f / 30.0f) / 5.06f, 0.0f, 1.0f) * 58.0f);
        if (x)
            cv_line(2 + x, py, 4 + x, y, ui.kview == 0u ? C_WHITE : C_AMB);   /* (white over the RGB) */
        py = y;
    }
    if (klab.cur.end || klab.cur.src) {  /* LOAF BREAKER: LAB's END (white) and STACK's hand-over (amber), where they
                                          * fall in the picture's 250 ms */
        uint32_t m;
        for (m = 0; m < 2u; m++) {
            float ms = m ? (klab.cur.src ? kick_lab_ms(KLM_CROSS, klab.cur.cross) : -1.0f)
                         : (klab.cur.end ? kick_lab_ms(KLM_END, klab.cur.end) : -1.0f);
            int32_t mx = 4 + (int32_t)(ms * ((float)KV_W / (float)KV_MS));
            if (ms >= 0.0f && mx < 4 + (int32_t)KV_W)
                for (x = 16; x < 74; x += 4)
                    cv_rect(mx, x, 1, 2, m ? C_AMB : C_WHITE);
        }
    }
    t[0] = 0;                            /* KICK 4, its note */
    text_cat(t, "KICK ");
    int_text((int32_t)set.kslot + 1, u);
    text_cat(t, u);
    cv_text(6, 2, &FONT_B, t, C_WHITE);
    {                                    /* LOAF BREAKER TEST: what the part is for, in a few words (K SET: the knob and
                                          * its function, "K4 TERROR": the tabs leave it no room) */
        static const char *const D[32] = {"THE 4 LAYERS' LEVELS", "THE LOW BOOM", "THE SUB'S EFFECT", "THE THUMP",
                                          "THE PUNCH'S EFFECT", "THE DIRTY LAYER", "THE GRIT'S EFFECT", "THE ATTACK",
                                          "THE CLICK'S EFFECT", "ALL LAYERS AT ONCE", "VOLUME, BUILT-IN KICKS", "",
                                          "", "", "", "FOLD DISTORTION", "THE DISTORTION'S DETAIL",
                                          "MIDS PUSHED, SUB KEPT", "PITCH DROP AND BODY", "RUMBLE, TUNE",
                                          "GLIDE, REVERSE BASS", "WHAT SEL + K1-K4 DO", "", "", "", "",
                                          "ROUND SUB, BLEND KICKS", "", "THE TAIL", "TAIL DARKER, HARDER",
                                          "ATTACK + TAIL", "MUTATE, A/B, VIEWS"};
        uint32_t p = kick_part();
        t[0] = 0;
        if (p >= KP_BASE + 11u && p <= KP_BASE + 14u)
            kick_part_name(p, t);
        else if (p == KP_BASE + 16u) {   /* (STYLE: the one the knobs have now; PRESETS picks one) */
            text_cat(t, "PRESETS: ");   /* (0.9.66: PRESETS alone there) */
            text_cat(t, kstyle_name());
        }
        else if (kick_easy())            /* (BEGINNER's pages) */
            text_cat(t, keasy_pg == 2u ? "THE FOUR LAYERS' LEVELS" : keasy_pg ? "THE PITCH DROP, THE TAIL"
                                                                                : "PRESET, DISTO, DRIVE");
        else if (p == KP_PART_PRESET)
            text_cat(t, "THE BUILT-IN KICKS");
        else if (p < 32u)
            text_cat(t, D[p]);
        cv_text(12 + text_w(&FONT_B, "KICK 10"), 4, &FONT_XS, t, kp_knobs() || ui.kpage == KP_STYLE ? C_HI : C_GRAY);
    }
    t[0] = 0;
    int_text((int32_t)(hz + 0.5f), t);
    text_cat(t, " HZ");
    cv_text(234 - text_w(&FONT_XS, t), 4, &FONT_XS, t, C_AMB);
    if (kv_ms() != (ui.kview == 0u ? 500u : 250u)) {   /* (LOAF BREAKER: KICK VIEW PART or EDIT ZOOM: the picture's
                                          * time, under the pitch: the right end, the tail's quiet) */
        uint32_t ms = kv_ms();
        t[0] = 0;
        if (ms >= 1000u) {               /* ("1.5 S") */
            int_text((int32_t)(ms / 1000u), t);
            if (ms % 1000u)
                text_cat(t, ".5");
            text_cat(t, " S");
        } else {
            int_text((int32_t)ms, t);
            text_cat(t, " MS");
        }
        cv_text(234 - text_w(&FONT_XS, t), 13, &FONT_XS, t, C_AMB);
    }
    {                                    /* LOAF BREAKER (0.205; 0.207 a bar): the section's parts at the foot, as the
                                          * tabs: the whole width, the one open lit (BEGINNER: those it passes over
                                          * dim); the tabs' letters if they fit, else smaller */
        uint32_t j;
        int32_t cx = 4, y = 80;
        if (set.novice) {                /* (BEGINNER: its pages, KICK, SHAPE, MIX, as tabs; EDIT the next) */
            for (j = 0; j < KE_PAGES; j++) {
                int32_t x = cx + (int32_t)j * 64;
                cv_rect(x, y, 60, 15, j == keasy_pg ? C_HI : C_DIM);
                cv_text(x + 30 - text_w(&FONT_T, KE_PAGE[j]) / 2, y + 2, &FONT_T, KE_PAGE[j],
                        j == keasy_pg ? C_BLACK : C_GRAY);
            }
            cv_text(cx + 198, y + 4, &FONT_XS, "EDIT", C_DIM);
        } else
            ksect_row(y);
    }
    cv_oy = 0;
}
static void kick_cards(void)             /* the part's four knobs: its name, its value large, its bar */
{
    uint32_t i;
    char t[16];
    cv_begin(240, 48, C_BLACK);
    for (i = 0; i < 4u; i++) {
        int32_t x = 2 + (int32_t)i * 60;
        const char *n = kick_knob_name((int)i);
        cv_rect(x, 0, 56, 46, C_BLOCK);
        if (!n[0])
            continue;
        if (ui.kpage == KP_STYLE) {      /* (LOAF BREAKER: STYLE: the function's value too, by its knob's name; too
                                          * long for both, the value alone) */
            selk_value(selk_fn(i), 0, t);
            if (text_w(&FONT_XS, t) <= 34)
                cv_text(x + 4, 3, &FONT_XS, n, C_GRAY);
            cv_text(x + 52 - text_w(&FONT_XS, t), 3, &FONT_XS, t, C_WHITE);
        } else
            cv_text(x + 4, 3, &FONT_XS, n, C_GRAY);
        kick_value((int)i, t);
        cv_text(x + 4, 16, text_w(&FONT_B, t) <= 50 ? &FONT_B : &FONT_XS, t, C_WHITE);
        cv_rect(x + 4, 38, 48, 3, C_BLACK);
        cv_rect(x + 4, 38, (int32_t)(48.0f * brk_clampf(kick_knob_f((int)i), 0.0f, 1.0f) + 0.5f), 3, C_HI);
    }
    cv_blit(0, 144);
}
static void draw_kick_classic(const brk_status_t *st)   /* LOAF BREAKER: the menu as it was (EDIT held: either):
                                          * the part's knobs as bars, the layers' levels, the five drums' shapes */
{
    const kick_t *k = &set.kick;
    uint32_t i;
    char t[24];
    cv_begin(240, 52, C_BLACK);
    t[0] = 0;
    text_cat(t, "KICK  ");
    kick_part_name(kick_part(), t);
    cv_text(6, 0, &FONT_XS, t, C_GRAY);
    for (i = 0; i < 4u; i++) {           /* the part's four settings */
        int32_t x = 6 + (int32_t)i * 58;
        const char *n = kick_knob_name((int)i);
        if (!n[0])
            continue;
        cv_text(x, 16, &FONT_XS, n, C_GRAY);
        cv_rect(x, 29, 52, 3, C_BLOCK);
        cv_rect(x, 29, (int32_t)(52.0f * brk_clampf(kick_knob_f((int)i), 0.0f, 1.0f) + 0.5f), 3, C_HI);
    }
    for (i = 0; i < KICK_LAYERS; i++) {  /* the layers' levels */
        int32_t x = 6 + (int32_t)i * 58;
        int lit = i == ui.klayer && (ui.kpage == KP_LAYER || ui.kpage == KP_FX);   /* (MIX: the row above is them) */
        cv_text(x, 34, &FONT_XS, KICK_LAYER_NAME[i], lit ? C_GRAY : C_DIM);
        cv_rect(x, 47, 52, 3, C_BLOCK);
        cv_rect(x, 47, (int32_t)(52.0f * k->l[i].level + 0.5f), 3, lit ? C_HI : k->l[i].fx ? C_AMB : C_DIM);
    }
    cv_blit(0, 88);
    draw_drums(st);
}
static void draw_kick(const brk_status_t *st)
{
    uint32_t sig = sig_of(&set.kick, sizeof set.kick) ^ sig_of(set.ksyn, sizeof set.ksyn) ^
                   (sig_of(&klab.cur, sizeof klab.cur) ^ sig_of(kx2_at(set.kslot), sizeof(kick_x2_t)) * 5u) * 3u ^ kv_part_sig() * 7u, top;   /* (LOAF BREAKER: its LAB too, the
                                          * part shown) */
    kick_t kk;
    if (ui.kclassic) {
        draw_kick_classic(st);
        return;
    }
    if (sig != kv_sig && plat_ms() - kv_t < 100u)
        kv_pend = 1u;                    /* (TEST: a knob turning: the hit again at most every 100 ms, the last one
                                          * drawn then: main.c) */
    else if (sig != kv_sig) {            /* (its hit again, only when it changed) */
        uint32_t pk = 0u;
        kv_sig = sig;
        kv_t = plat_ms();
        kv_pend = 0u;
        kv_kick(&kk);
        if (ui.kview == 0u)              /* (LOAF BREAKER: the RGB view, its peaks its own; or the wave over its level) */
            kick_rgb(&kk, kv_rgb, KV_W, KV_MS);
        else {
            kick_shape(&kk, kv_px, KV_W, KV_MS, 29u);
            kick_wave(&kk, kv_wave, KV_W, KV_MS, 29u);
        }
        for (top = 0; top < KV_W; top++)
            pk = kv_px[top] > pk ? kv_px[top] : pk;
        for (top = 0; pk && top < KV_W; top++) {   /* (as tall as it goes) */
            kv_px[top] = (uint8_t)((uint32_t)kv_px[top] * 29u / pk);
            kv_wave[top] = (int8_t)((int32_t)kv_wave[top] * 29 / (int32_t)pk);
        }
    }
    for (top = 0; top < 100u; top += 60u) {   /* (y 44..143, strips of 60 rows) */
        uint32_t h = 100u - top < 60u ? 100u - top : 60u;
        cv_begin(240, h, C_BLACK);
        kick_picture((int32_t)top);
        cv_blit(0, 44u + top);
    }
    kick_cards();
}

/* LOAF BREAKER: HOME's third part, the KICK EQ: the FX chain's EQ on the kick alone (its curve drawn as the FX page
 * draws it). K1-K4: LOW, MID, HIGH, FREQ; ALGORITHM, their shape: LO FREQ, MID Q, HI FREQ, LEVEL */
static const char *const KEQ_NAME[2][4] = {{"LOW", "MID", "HIGH", "MID FREQ"}, {"LO FREQ", "MID Q", "HI FREQ", "LEVEL"}};
static void keq_value(int i, char *t)
{
    uint32_t j = (ui.keq_part & 1u) * 4u + (uint32_t)(i & 3);
    float v = set.keq[j];
    t[0] = 0;
    if (j < 3u || j == 7u) {             /* -12 .. +12 dB, flat in the middle */
        float db = (v - 0.5f) * 24.0f;
        if (db > -0.25f && db < 0.25f)
            text_cat(t, "0 DB");
        else
            signed_tenths(db, "DB", t);
    } else if (j == 5u)
        hundredths_text(0.3f * brk_exp2f(v * 3.737f), t);
    else
        hz_text(j == 3u ? 200.0f * brk_exp2f(v * 4.64f) : j == 4u ? 40.0f * brk_exp2f(v * 3.3219f)
                                                        : 2000.0f * brk_exp2f(v * 3.0f), t);
}
static void keq_turn(int i, int32_t d)
{
    uint32_t j = (ui.keq_part & 1u) * 4u + (uint32_t)(i & 3);
    char t[16];
    set.keq[j] = knob_f(set.keq[j], d, 0.02f, 0.0f, 1.0f);
    changed_kick();
    keq_value(i, t);
    app_say(KEQ_NAME[ui.keq_part & 1u][i & 3], t);
    ui.redraw |= RG_BLOCKS;
}
static void draw_keq(void)
{
    const float *k = set.keq;
    float p[4], xx[4];
    int32_t x, y, yb, py = 0;
    uint32_t j;
    char t[16];
    for (j = 0; j < 4u; j++) {
        p[j] = k[j];
        xx[j] = k[4u + j];
    }
    cv_begin(240, 52, C_BLACK);          /* (0.9.2: the curve, then the FX section's parts as its other sections') */
    for (x = 0; x < 240; x += 4)         /* 0 dB */
        cv_rect(x, 18, 2, 1, C_LINE);
    for (x = 0; x < 240; x++) {          /* the curve, as the FX page's EQ draws it */
        float in, base, v = x_plot(FXT_EQ, x, p, xx, 0u, &in, &base);
        y = 18 - (int32_t)(brk_clampf(v, -1.0f, 1.0f) * 15.0f);
        yb = 18 - (int32_t)(base * 15.0f);
        cv_rect(x, y < yb ? y : yb, 1, y < yb ? yb - y : y - yb, MB_DIM[1]);
        if (x)
            cv_line(x - 1, py, x, y, MB_COL[1]);
        py = y;
    }
    if (keq_flat)                        /* (0.9.57: left out, SYSTEM › SETUP K2) */
        cv_text(4, 1, &FONT_XS, "OFF: SYSTEM > SETUP K2 (FLAT)", C_AMB);
    ksect_row(36);
    cv_blit(0, 88);
    cv_begin(240, 52, C_BLACK);
    for (j = 0; j < 8u; j++) {           /* the eight settings, the part K1-K4 turn lit */
        int lit = (j / 4u) == (ui.keq_part & 1u);
        int32_t px = 6 + (int32_t)(j % 4u) * 58, pyy = 2 + (int32_t)(j / 4u) * 25;
        uint8_t save = ui.keq_part;
        ui.keq_part = (uint8_t)(j / 4u);
        keq_value((int)(j % 4u), t);
        ui.keq_part = save;
        cv_text(px, pyy, &FONT_XS, KEQ_NAME[j / 4u][j % 4u], lit ? C_GRAY : C_DIM);
        cv_text(px, pyy + 10, &FONT_T, t, lit ? C_WHITE : C_DIM);
    }
    cv_blit(0, 140);
}

/* LOAF BREAKER: the KICK LAB's pages (klab.cur, the kick playing's; kick.h kick_lab_t says what each does) */
static const char *const KL_CURVE[3] = {"EXP", "LINEAR", "SNAP"};
static const char *const KL_SOLO[3] = {"BOTH", "ATTACK", "TAIL"};
/* TOOLS: MUTATE (right: the kick's layers and INTENSITY moved a little at random, again and again; left: back to
 * before the first), A/B (this kick against itself as it came or was last saved: ab_take) */
static kick_t mut_from;
static uint8_t mut_n;
static uint32_t mut_rng = 0x2545F491u;
static kick_t ab_k;
static int8_t ab_syn[KS_N];
static kick_lab_t ab_lab;
static uint8_t ab_b;                     /* 1: B, the kick as it came, playing (A, the edited one, kept in ab_*) */
static void ab_take(void)
{
    uint32_t j;
    ab_k = set.kick;
    for (j = 0; j < KS_N; j++)
        ab_syn[j] = set.ksyn[j];
    ab_lab = klab.cur;
    ab_b = 0u;
    mut_n = 0u;
}
static void klab_value(int i, char *t)
{
    const kick_lab_t *l = &klab.cur;
    t[0] = 0;
    switch (ui.kpage * 4 + i) {
    case KP_TAIL * 4:
        text_cat(t, KTM_NAME[l->mode % KTM_N]);
        break;
    case KP_TAIL * 4 + 1:
        if (l->note)
            note_name((int32_t)l->note + 22 - 48, t);   /* (seq.c's: 0 is C3, MIDI 48) */
        else
            text_cat(t, "FREE");
        break;
    case KP_TAIL * 4 + 2:
        text_cat(t, KL_CURVE[l->curve % 3u]);
        break;
    case KP_TAIL * 4 + 3:
        if (l->end)
            ms_text(kick_lab_ms(KLM_END, l->end), t);
        else
            text_cat(t, "OFF");
        break;
    case KP_TAIL2 * 4:
        if (l->filt)
            hz_text(kick_lab_ms(KLM_FILT, l->filt), t);
        else
            text_cat(t, "OFF");
        break;
    case KP_TAIL2 * 4 + 1:
        ms_text(kick_lab_ms(KLM_FTIME, l->ftime), t);
        break;
    case KP_TAIL2 * 4 + 2:
        if (l->drive) {
            int_text(l->drive, t);
            text_cat(t, "%");
        } else
            text_cat(t, "OFF");
        break;
    case KP_TAIL2 * 4 + 3:               /* (0.9.71: SPLIT's time, DIRTY's rise; nothing for CLASSIC, CLEAN) */
        if (l->mode == KTM_SPLIT || l->mode == KTM_DIRTY)
            ms_text(kick_lab_ms(l->mode == KTM_SPLIT ? KLM_SPLIT : KLM_RISE, l->amt), t);
        break;
    case KP_STACK * 4:
        if (l->src) {
            char u[4];
            text_cat(t, "KICK ");
            int_text(l->src, u);
            text_cat(t, u);
        } else
            text_cat(t, "OFF");
        break;
    case KP_STACK * 4 + 1:
        ms_text(kick_lab_ms(KLM_CROSS, l->cross), t);
        break;
    case KP_STACK * 4 + 2:
        ms_text(kick_lab_ms(KLM_FADE, l->fade), t);
        break;
    case KP_STACK * 4 + 3:
        text_cat(t, KL_SOLO[klab_solo % 3u]);
        break;
    case KP_TOOLS * 4:
        if (mut_n) {
            text_cat(t, "X");
            int_text(mut_n, t + 1);
        } else
            text_cat(t, "TURN");
        break;
    case KP_TOOLS * 4 + 1:
        text_cat(t, ab_b ? "B SAVED" : "A EDIT");
        break;
    case KP_TOOLS * 4 + 2:               /* (0.9.29, FLEXOR: EDIT VIEW and DRUM VIEW ZOOM here, SYSTEM › EFFECTS before) */
        text_cat(t, set.kfocus ? "PART" : "WHOLE");
        break;
    case KP_TOOLS * 4 + 3:
        text_cat(t, EZ_NAME[ez_idx()]);
        break;
    default:
        break;
    }
}
static float klab_norm(int i)
{
    const kick_lab_t *l = &klab.cur;
    switch (ui.kpage * 4 + i) {
    case KP_TAIL * 4: return (float)(l->mode % KTM_N) * (1.0f / (float)(KTM_N - 1u));
    case KP_TAIL * 4 + 1: return (float)l->note * (1.0f / 24.0f);
    case KP_TAIL * 4 + 2: return (float)(l->curve % 3u) * 0.5f;
    case KP_TAIL * 4 + 3: return (float)l->end * (1.0f / (float)KICK_END_MAX);
    case KP_TAIL2 * 4: return (float)l->filt * 0.01f;
    case KP_TAIL2 * 4 + 1: return (float)l->ftime * 0.01f;
    case KP_TAIL2 * 4 + 2: return (float)l->drive * 0.01f;
    case KP_TAIL2 * 4 + 3: return l->mode == KTM_SPLIT || l->mode == KTM_DIRTY ? (float)l->amt * 0.01f : 0.0f;
    case KP_STACK * 4: return (float)l->src * 0.1f;
    case KP_STACK * 4 + 1: return (float)l->cross * 0.01f;
    case KP_STACK * 4 + 2: return (float)l->fade * 0.01f;
    case KP_STACK * 4 + 3: return (float)(klab_solo % 3u) * 0.5f;
    case KP_TOOLS * 4: return (float)(mut_n > 20u ? 20u : mut_n) * 0.05f;
    case KP_TOOLS * 4 + 1: return (float)ab_b;
    case KP_TOOLS * 4 + 2: return (float)set.kfocus;
    case KP_TOOLS * 4 + 3: return (float)ez_idx() / (float)(EZ_N - 1);
    default: return 0.0f;
    }
}
static void mutate(int32_t d)
{
    kick_t *k = &set.kick;
    uint32_t i, j;
    if (d < 0) {                         /* (back to before the first) */
        float vol = k->volume;
        if (!mut_n) {
            app_say("MUTATE", "NOTHING TO UNDO");
            return;
        }
        *k = mut_from;
        k->volume = vol;
        mut_n = 0u;
        app_say("MUTATE", "UNDONE");
        return;
    }
    if (!mut_n)
        mut_from = *k;
    for (i = 0; i < KICK_LAYERS; i++)
        for (j = 0; j < 4u; j++) {       /* (level, pitch, decay, tone: +-0.04 each) */
            float *p = j == 0u ? &k->l[i].level : j == 1u ? &k->l[i].pitch : j == 2u ? &k->l[i].decay : &k->l[i].tone;
            mut_rng = mut_rng * 1664525u + 1013904223u;
            *p = brk_clampf(*p + ((float)(mut_rng >> 9) * (1.0f / 8388608.0f) - 0.5f) * 0.08f, 0.0f, 1.0f);
        }
    mut_rng = mut_rng * 1664525u + 1013904223u;
    k->intensity = brk_clampf(k->intensity + ((float)(mut_rng >> 9) * (1.0f / 8388608.0f) - 0.5f) * 0.06f, 0.0f, 1.0f);
    if (mut_n < 99u)
        mut_n++;
    {
        char t[16] = "X";
        int_text(mut_n, t + 1);
        text_cat(t, " (LEFT: UNDO)");
        app_say("MUTATE", t);
    }
}
static void ab_swap(void)
{
    kick_t k = set.kick;
    kick_lab_t l = klab.cur;
    float vol = set.kick.volume;
    uint32_t j;
    set.kick = ab_k;
    set.kick.volume = vol;
    ab_k = k;
    for (j = 0; j < KS_N; j++) {
        int8_t s = set.ksyn[j];
        set.ksyn[j] = ab_syn[j];
        ab_syn[j] = s;
    }
    klab.cur = ab_lab;
    ab_lab = l;
    ab_b ^= 1u;
    app_say("A/B", ab_b ? "B: AS SAVED" : "A: AS EDITED");
}
static void klab_turn(int i, int32_t d)
{
    kick_lab_t *l = &klab.cur;
    char t[16];
    int32_t s = d > 0 ? 1 : -1;
    switch (ui.kpage * 4 + i) {
    case KP_TAIL * 4:
        ktm_turn(l, d);
        break;
    case KP_TAIL * 4 + 1:
        l->note = (uint8_t)clamp((int32_t)l->note + s, 0, 24);
        break;
    case KP_TAIL * 4 + 2:
        l->curve = (uint8_t)clamp((int32_t)(l->curve % 3u) + s, 0, 2);
        break;
    case KP_TAIL * 4 + 3:
        l->end = (uint8_t)clamp((int32_t)l->end + d, 0, (int32_t)KICK_END_MAX);
        break;
    case KP_TAIL2 * 4:
        if (!l->filt && d > 0 && !l->ftime)
            l->ftime = 50u;              /* (the first time: closing over ~110 ms) */
        l->filt = (uint8_t)clamp((int32_t)l->filt + d * 2, 0, 100);
        break;
    case KP_TAIL2 * 4 + 1:
        l->ftime = (uint8_t)clamp((int32_t)l->ftime + d * 2, 0, 100);
        break;
    case KP_TAIL2 * 4 + 2:
        l->drive = (uint8_t)clamp((int32_t)l->drive + d * 2, 0, 100);
        break;
    case KP_TAIL2 * 4 + 3:
        if (l->mode == KTM_SPLIT || l->mode == KTM_DIRTY)
            l->amt = (uint8_t)clamp((int32_t)l->amt + d * 2, 0, 100);
        break;
    case KP_STACK * 4:
        if (!l->src && d > 0 && !l->cross && !l->fade) {
            l->cross = 50u;              /* (the first time: handed over at ~55 ms, in ~11 ms) */
            l->fade = 40u;
        }
        l->src = (uint8_t)clamp((int32_t)l->src + s, 0, (int32_t)KICK_MEMS);
        break;
    case KP_STACK * 4 + 1:
        l->cross = (uint8_t)clamp((int32_t)l->cross + d * 2, 0, 100);
        break;
    case KP_STACK * 4 + 2:
        l->fade = (uint8_t)clamp((int32_t)l->fade + d * 2, 0, 100);
        break;
    case KP_STACK * 4 + 3:
        klab_solo = (uint8_t)clamp((int32_t)(klab_solo % 3u) + s, 0, 2);
        break;
    case KP_TOOLS * 4:
        mutate(d);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    case KP_TOOLS * 4 + 1:
        if ((d > 0) != (ab_b != 0u))     /* (right: B, left: A) */
            ab_swap();
        else
            app_say("A/B", ab_b ? "B: AS SAVED" : "A: AS EDITED");
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        changed_kick();
        return;
    case KP_TOOLS * 4 + 2:               /* EDIT VIEW: the picture, left the WHOLE kick, right the PART edited */
        set.kfocus = d > 0 ? 1u : 0u;
        changed();
        app_say("EDIT VIEW", set.kfocus ? "THE PART EDITED" : "THE WHOLE KICK");
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    case KP_TOOLS * 4 + 3:               /* DRUM VIEW ZOOM: the time EDIT's pictures show (the kick, the drums) */
        set.ezoom = (uint8_t)(EZ_TAG | (uint32_t)clamp((int32_t)ez_idx() + (d > 0 ? 1 : -1), 0, EZ_N - 1));
        changed();
        app_say("DRUM VIEW ZOOM", ez_idx() ? EZ_NAME[ez_idx()] : "AUTO (EACH PART ITS OWN)");
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    default:
        return;
    }
    klab_value(i, t);
    app_say(kick_knob_name(i), t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    changed_kick();
}
