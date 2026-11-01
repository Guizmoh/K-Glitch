/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH: the rhythm itself. Included by kglitch.c.
 *
 * DRUMS (FX > DRUMS): a two-bar drum pattern written on the drum track in a style (JUNGLE, BREAKCORE, DNB, DRILL, IDM,
 * GLITCH, HALFTIME, FOOTWORK), at its tempo and with a kit that suits it; VARIATION mutates it a little at once.
 * What it writes becomes the ORIGINAL (CHOP's RESTORE and CLEAR come back to it).
 *
 * CHOP (FX > CHOP): MUTATE rewrites the pattern from the ORIGINAL (never from a mutation of a mutation: it never
 * drifts away), CHAOS and DENSITY say how much:
 *   delete    hits taken out (CHAOS); the first kick of the bar stays until CHAOS 70 %
 *   shift     hits moved to the next or previous 16th, or nudged early / late (SLOOP's micro timing) (CHAOS)
 *   repeat    fragments of 2 or 4 16ths copied over others (DENSITY)
 *   rolls     ratchets on snares, hats and rides (DENSITY)
 *   ghosts    ghost snares and hats added in the gaps (DENSITY)
 *   accents   some hits harder, some softer (CHAOS)
 *   lengths   (TARGET ALL) the synth tracks' notes made longer (a tie) or shorter (CHAOS)
 * EVERY mutates again on its own each 1, 2, 4 or 8 bars, while PLAY runs (the audio interrupt, on the bar).
 * A pattern changed by hand since the last mutation becomes the new ORIGINAL at the next MUTATE.
 * The ORIGINAL is kept in RAM only (about 0.9 KB): not in the project. */

enum { KS_JUNGLE, KS_BREAKCORE, KS_DNB, KS_DRILL, KS_IDM, KS_GLITCH, KS_HALFTIME, KS_FOOTWORK, KS_N };
typedef struct {
    uint8_t bpm, kit;                 /* kit: DRUM_SAMPLED + its place in DS_KIT_NAME_LIST */
    uint8_t lane[5];                  /* 0xFF: unused */
    const char *hit[5];               /* 32 16ths each: X hard, x normal, s soft, g ghost, r a ratchet x2, R x4, . none */
} kg_style_t;
static const kg_style_t KG_STYLE[KS_N] = {
    /* JUNGLE: the amen's phrasing, a ride in 8ths */
    {170, 17, {0, 2, 8, 12, 4}, {"x.x.....x.x.....x.x.....x.......", "....X.......X.......X.......X...",
                                 ".......g.g.....g.......g.g...g.g", "x...x...x...x...x...x...x...x.x.",
                                 "..s...s...s...s...s...s...s..rs."}},
    /* BREAKCORE: the amen cut up, snare rolls, a crash on the one */
    {190, 29, {0, 2, 4, 11, 8}, {"x..x..x.x.x...x.x.x..x....x.x...", "....X..X..R.X...X...X..gR...X.RR",
                                 "x.x.x.x.x.x.x.x.xRx.x.x.x.x.x.Rx", "x...............................",
                                 ".g.....g...g..g..g..g.....g..g.."}},
    /* DNB: the two-step */
    {174, 17, {0, 2, 4, 8, 0xFF}, {"x.........x.....x.........x..x..", "....X.......X.......X.......X...",
                                   "x.xgx.x.x.xgx.x.x.xgx.x.x.xgx.xr", "..........g.......g.....g.....g.", 0}},
    /* DRILL: sparse kicks, the snare on 3, hats with rolls, rims */
    {142, 6, {0, 2, 4, 7, 0xFF}, {"x......x..x.......x...x....x....", "........X...............X.....X.",
                                  "x.x.x.xRx.x.x.x.x.x.xRx.x.x.xRx.", "...........x...........x..x.....", 0}},
    /* IDM: off the grid, ghosts, congas */
    {128, 27, {0, 2, 4, 14, 8}, {"x...x..x.x....x.x..x...x..x..x..", "....X.......X..g...X.......X...",
                                 "x.gxx.g.xgx.xx.gx.g.xxg.x.gxR.x.", "..x.....x..x.....x......x...x...",
                                 "..........g......g.........g..g."}},
    /* GLITCH: clicks, rims, ratchets everywhere */
    {120, 27, {0, 2, 4, 7, 13}, {"x.......x..x....x....x..x.......", "....X.......X..R....X.......X.R.",
                                 "R.x.R..xx.R.x..RR.x..R.xx.R..x.R", "..x...x....x..x...x...x..x....x.",
                                 ".r.....r.....r.....r.....r...r.."}},
    /* HALFTIME: the snare on 3, ghosts, a busy hat */
    {160, 18, {0, 2, 4, 8, 0xFF}, {"x.........x.....x.....x...x.....", "........X...............X.......",
                                   "x.x.x.x.x.x.x.x.x.x.x.x.x.x.xRxR", "...g.....g...g.....g.....g...g..", 0}},
    /* FOOTWORK: kicks in threes against the claps */
    {160, 5, {0, 3, 4, 7, 0xFF}, {"x..x..x...x..x..x..x..x...x..x..", "....X.......X.......X.......X...",
                                  "..x...x...x...x...x...x...x..RxR", "x.....x.....x....x.....x....x...", 0}},
};
static const char *const N_KSTYLE[KS_N + 1] = {"JUNGLE", "BRKCORE", "DNB", "DRILL", "IDM", "GLITCH", "HALFTM", "FTWORK", 0};

static struct {
    dstep_t d[NSTEP];                 /* the ORIGINAL drum pattern */
    int8_t micro[NSTEP];
    uint8_t time[NPART][NSTEP];       /* the synth tracks' steps: ST_NOTE / ST_TIE / ST_REST */
    uint8_t len;                      /* its drum length */
    uint8_t has;                      /* an ORIGINAL is kept */
    uint8_t mutated;                  /* what plays is a mutation of it */
    uint32_t out_sig;                 /* the pattern as the last mutation left it (a change by hand since: a new ORIGINAL) */
    uint32_t rnd;
    uint32_t bar;                     /* EVERY: the bar last seen */
    uint16_t changed;                 /* hits the last mutation changed (the screen) */
} kr = {.rnd = 0x7F4A7C15u, .bar = 0xFFFFFFFFu};

static uint32_t kr_sig(void)                      /* the drum pattern and the synth steps' lengths, hashed */
{
    uint32_t h = 2166136261u, i, t;
    const uint8_t *p = (const uint8_t *)TDRUM->dstep;
    for (i = 0; i < sizeof(dstep_t) * NSTEP; i++) h = (h ^ p[i]) * 16777619u;
    for (i = 0; i < NSTEP; i++) h = (h ^ (uint8_t)TDRUM->micro[i]) * 16777619u;
    for (t = 0; t < NPART; t++)
        for (i = 0; i < NSTEP; i++) h = (h ^ trk[t].step[i].time) * 16777619u;
    return h ^ (uint32_t)TDRUM->p[P_SLEN];
}
static void kr_keep(void)                         /* what plays becomes the ORIGINAL */
{
    uint32_t i, t;
    for (i = 0; i < NSTEP; i++) {
        kr.d[i] = TDRUM->dstep[i];
        kr.micro[i] = TDRUM->micro[i];
        for (t = 0; t < NPART; t++) kr.time[t][i] = trk[t].step[i].time;
    }
    kr.len = (uint8_t)TDRUM->p[P_SLEN];
    kr.has = 1;
    kr.mutated = 0;
    kr.changed = 0;
}
static void kr_restore(void)                      /* back to the ORIGINAL (CHOP RESTORE, CLEAR) */
{
    uint32_t i, t;
    if (!kr.has) return;
    for (i = 0; i < NSTEP; i++) {
        TDRUM->dstep[i] = kr.d[i];
        TDRUM->micro[i] = kr.micro[i];
        for (t = 0; t < NPART; t++) trk[t].step[i].time = kr.time[t][i];
    }
    TDRUM->p[P_SLEN] = kr.len;
    kr.mutated = 0;
    kr.changed = 0;
    kr.out_sig = kr_sig();
}

/* a mutation of the ORIGINAL; all: the synth tracks' lengths too. Main loop (interrupts off) or the audio interrupt */
static void kr_mutate(int all)
{
    uint32_t C = kg_pct(G_KCHAOS), D = kg_pct(G_KDENS), len, s, l, n, changed = 0;
    if (!kr.has || kr_sig() != kr.out_sig)        /* none yet, or changed by hand: this is the ORIGINAL now */
        kr_keep();
    len = kr.len ? kr.len : 16u;
    if (len > NSTEP) len = NSTEP;
    for (s = 0; s < NSTEP; s++) {                  /* from the ORIGINAL */
        TDRUM->dstep[s] = kr.d[s];
        TDRUM->micro[s] = kr.micro[s];
    }
    for (s = 0; s < len; s++)
        for (l = 0; l < DRUM_LANES; l++) {
            dstep_t *d = &TDRUM->dstep[s];
            uint32_t r = kg_rand(&kr.rnd), lv;
            if (!dstep_has(&kr.d[s], l)) continue;
            lv = dstep_lvl(&kr.d[s], l);
            if (r % 100u < C / 4u && !(s % 16u == 0u && l == 0u && C <= 70u)) {   /* delete */
                dstep_clr(d, l);
                changed++;
            } else if ((r >> 8) % 100u < C / 3u) {     /* shift: to a neighbour, or nudged */
                uint32_t to = (s + ((r >> 16) & 1u ? 1u : len - 1u)) % len;
                if ((r >> 17) & 1u && !dstep_has(&TDRUM->dstep[to], l)) {
                    dstep_clr(d, l);
                    dstep_set(&TDRUM->dstep[to], l, lv, 0);
                } else {
                    TDRUM->micro[s] = (int8_t)clamp((int32_t)((r >> 20) % (C * 24u / 100u + 1u)) * ((r >> 19) & 1u ? 1 : -1),
                                                     MICRO_MIN, MICRO_MAX);
                }
                changed++;
            } else if ((r >> 12) % 100u < C / 5u) {    /* accent */
                dstep_set(d, l, (r >> 24) & 1u ? LV_HARD : LV_SOFT, 0);
                changed++;
            }
        }
    for (n = D / 20u; n > 0u; n--) {               /* repeat fragments */
        uint32_t r = kg_rand(&kr.rnd), w = (r & 1u) ? 4u : 2u, from, to, i;
        if (len < 2u * w) continue;
        from = ((r >> 4) % (len / w)) * w, to = ((r >> 12) % (len / w)) * w;
        if (from == to) continue;
        for (i = 0; i < w; i++) TDRUM->dstep[to + i] = TDRUM->dstep[from + i];
        changed += w;
    }
    for (s = 0; s < len; s++) {                    /* rolls, ghosts */
        dstep_t *d = &TDRUM->dstep[s];
        static const uint8_t ROLL[4] = {2, 4, 8, 12};
        uint32_t r = kg_rand(&kr.rnd), i;
        for (i = 0; i < 4u; i++)
            if (dstep_has(d, ROLL[i]) && (r >> (i * 7u)) % 100u < D / 5u) {
                dstep_set(d, ROLL[i], dstep_lvl(d, ROLL[i]), 1u + (r >> 28) % 3u);
                changed++;
            }
        if (!d->on[0] && !d->on[1] && kg_rand(&kr.rnd) % 100u < D / 8u) {
            dstep_set(d, (kg_rand(&kr.rnd) & 1u) ? 8u : 4u, LV_GHOST, 0);
            changed++;
        }
    }
    if (all) {                                     /* the synth tracks: notes longer or shorter */
        uint32_t t;
        for (t = 0; t < NPART; t++) {
            uint32_t sl = (uint32_t)trk[t].p[P_SLEN];
            if (sl > NSTEP) sl = NSTEP;
            for (s = 0; s < NSTEP; s++) trk[t].step[s].time = kr.time[t][s];
            for (s = 0; s + 1u < sl; s++) {
                step_t *st = &trk[t].step[s], *nx = &trk[t].step[s + 1u];
                if ((st->time != ST_NOTE && st->time != ST_TIE) || kg_rand(&kr.rnd) % 100u >= C / 3u) continue;
                if (nx->time == ST_REST) nx->time = ST_TIE, changed++;
                else if (nx->time == ST_TIE) nx->time = ST_REST, changed++;
            }
        }
    }
    kr.mutated = 1;
    kr.changed = (uint16_t)changed;
    kr.out_sig = kr_sig();
}
static void kr_mutate_ui(void)                    /* MUTATE from the main loop */
{
    kr.rnd ^= fm1_ms * 2654435761u;
    fm1_irq_off();
    kr_mutate(song.g[G_KCTGT] != 0);
    fm1_irq_on();
}

/* DRUMS: a style's pattern, its tempo and kit; it becomes the ORIGINAL; VARIATION mutates it at once (main loop) */
static void kr_generate(uint32_t st)
{
    const kg_style_t *y = &KG_STYLE[st % KS_N];
    track_t *t = TDRUM;
    uint32_t i, j;
    fm1_irq_off();
    for (i = 0; i < NSTEP; i++) {
        memset(&t->dstep[i], 0, sizeof t->dstep[i]);
        t->micro[i] = 0;
    }
    for (j = 0; j < 5u; j++)
        if (y->lane[j] != 0xFFu && y->hit[j])
            for (i = 0; i < 32u && y->hit[j][i]; i++) {
                char c = y->hit[j][i];
                if (c == '.') continue;
                dstep_set(&t->dstep[i], y->lane[j],
                          c == 'X' ? LV_HARD : c == 's' ? LV_SOFT : c == 'g' ? LV_GHOST : LV_NORM,
                          c == 'r' ? 1u : c == 'R' ? 3u : 0u);
            }
    t->p[P_SLEN] = 32;
    t->p[P_E0] = (int16_t)(DRUM_SAMPLED + y->kit);
    song.g[G_BPM] = y->bpm;
    kr_keep();
    kr.out_sig = kr_sig();
    if (song.g[G_KVAR]) {                          /* VARIATION: a mutation at CHAOS = DENSITY = VARIATION / 2 */
        int16_t c = song.g[G_KCHAOS], d = song.g[G_KDENS];
        song.g[G_KCHAOS] = song.g[G_KDENS] = (int16_t)(song.g[G_KVAR] / 2);
        kr_mutate(0);
        kr_keep();                                 /* (the varied one is the ORIGINAL) */
        kr.out_sig = kr_sig();
        song.g[G_KCHAOS] = c, song.g[G_KDENS] = d;
    }
    fm1_irq_on();
}

/* the audio interrupt, once a block from kg_route while PLAY runs: EVERY and CHAOS mode mutate on the bar */
static void kr_bar(void)
{
    static const uint8_t EVERY[5] = {0, 1, 2, 4, 8};
    uint32_t bar = clk_beat / 4u, every = EVERY[(uint32_t)song.g[G_KEVERY] % 5u];
    if (bar == kr.bar) return;
    kr.bar = bar;
    if (!bar) return;                              /* (not on the very first downbeat) */
    if (every && bar % every == 0u)
        kr_mutate(song.g[G_KCTGT] != 0);
    else if (song.g[G_KCMODE] && kg_rand(&kr.rnd) % 100u < kg_pct(G_KCHAOS)) {   /* CHAOS: on the bar, CHAOS % */
        kr_mutate(song.g[G_KCTGT] != 0);
        if (kg_rand(&kr.rnd) % 100u < kg_pct(G_KCHAOS) / 2u)
            kg_mutate(kg_rand(&kr.rnd));           /* .. and sometimes a new effect sequence */
    }
}
