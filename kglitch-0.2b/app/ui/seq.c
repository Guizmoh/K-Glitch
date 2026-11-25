/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH: the pattern sequencer (SEQ), the drum lanes and REC. (Part of app.c's unity: included there in order.) */

/* ---- the pattern sequencer (SEQ) and the drum lanes: the white keys on the SEQ, KICK and SNARE / HAT pages, REC */
enum { SL_BREAK1, SL_BREAK2, SL_KICK, SL_KICK2, SL_KICK3, SL_KICK4, SL_SNARE, SL_SNARE2, SL_SNARE3, SL_SNARE4, SL_HAT,
       SL_HAT2, SL_HAT3, SL_HAT4, SL_CLAP, SL_CLAP2, SL_CLAP3, SL_CLAP4, NSEQL };   /* (0.9.30, FLEXOR: every drum four bars) */   /* (LOAF BREAKER, 0.209: the MELODY's two gone) */                /* GLOUPI BREAKS: the kick's four bars and the snare's two, a lane each;
                                          * LOAF BREAKER: the hat's two, the clap's two */
static const char *const SEQ_LANE_NAME[NSEQL] = {"STEPS 1-16", "STEPS 17-32", "KICK 1-16", "KICK 17-32", "KICK 33-48",
                                                 "KICK 49-64", "SNARE 1-16", "SNARE 17-32", "SNARE 33-48", "SNARE 49-64",
                                                 "HAT 1-16", "HAT 17-32", "HAT 33-48", "HAT 49-64", "CLAP 1-16",
                                                 "CLAP 17-32", "CLAP 33-48", "CLAP 49-64"};
#define SQ_BRK_MIN 18                    /* LOAF BREAKER (0.242, FLEXOR): a break row's pitch, the rows' average (the grid's
                                          * height over its rows), between this (its number inside its box) and 24 */
#define SQ_BRK_MAX 24
#define SQ_H 148                         /* the picture's rows (screen 44..191) */
#define SQ_X 30                          /* the steps begin (the names before) */
static uint32_t text_n(const char *t) { uint32_t n = 0u; while (t[n]) n++; return n; }   /* (no libc's strlen on the FM-1) */
static const char *const DRUM_NAME[NDRUMS] = {"KICK", "SNARE", "HAT", "CLAP"};
static const char *const DRUM_TAG[NDRUMS] = {"KICK", "SN", "HH", "CP"};   /* (EDIT's tabs) */
static const uint8_t SL_DR[NSEQL] = {0u, 0u, DR_KICK, DR_KICK, DR_KICK, DR_KICK, DR_SNARE, DR_SNARE, DR_SNARE, DR_SNARE,
                                     DR_HAT, DR_HAT, DR_HAT, DR_HAT, DR_CLAP, DR_CLAP, DR_CLAP, DR_CLAP};   /* a drum
                                          * lane's drum, and its bar */
static const uint8_t SL_BAR[NSEQL] = {0u, 1u, 0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u};
static int sl_notes(uint32_t sl)         /* LOAF BREAKER: a drum lane: its keys act as they come up, a step held its
                                          * own knobs (K1 its note: the kick's, K2 HITS, K3 CHANCE, K4 NUDGE) */
{
    return sl >= SL_KICK && sl < NSEQL;
}
static int sl_tuned(uint32_t sl) { return sl >= SL_KICK && sl <= SL_KICK4; }
static int sl_kick(uint32_t sl) { return sl >= SL_KICK && sl <= SL_KICK4; }
static uint32_t sl_drum(uint32_t sl) { return SL_DR[sl % NSEQL]; }
static uint32_t sl_get(const pat_t *p, uint32_t sl)   /* the 16 steps a drum lane of SEQ (SL_KICK..) edits */
{
    return drum_bar(p, sl_drum(sl), SL_BAR[sl % NSEQL]);
}
static void sl_set(pat_t *p, uint32_t sl, uint32_t m)
{
    drum_bar_set(p, sl_drum(sl), SL_BAR[sl % NSEQL], m & 0xFFFFu);
}
static uint32_t sl_bars(const pat_t *p, uint32_t sl)   /* the bars its drum plays now: the kick 4, 2 or 1, the rest 2 or 1 */
{
    return drum_bars(p, sl_drum(sl));
}
static int sl_active(const pat_t *p, uint32_t sl)   /* a lane of SEQ the pattern has (its drum's length) */
{
    return sl < SL_KICK || SL_BAR[sl % NSEQL] < sl_bars(p, sl);
}
static int sl_now(const brk_status_t *st, uint32_t sl)   /* the step playing in that lane's 16, -1 another bar or none */
{
    const pat_t *p = &pats.p[set.pattern % NPAT];
    if (!st->playing || st->bar64 < 0 || (uint32_t)(st->bar64 >> 4) % sl_bars(p, sl) != SL_BAR[sl % NSEQL])
        return -1;
    return st->bar64 & 15;
}
static void seq_lane_fix(void)           /* a lane the pattern hasn't (a length shortened, another pattern): its first */
{
    const pat_t *p = &pats.p[set.pattern % NPAT];
    if (!sl_active(p, ui.seq_lane))
        ui.seq_lane = (uint8_t)(ui.seq_lane - SL_BAR[ui.seq_lane % NSEQL]);
}
static uint32_t seq_last_lane(void)      /* the pattern's last lane (its clap's, or the one before) */
{
    uint32_t sl = NSEQL - 1u;
    while (sl > SL_BREAK1 && !sl_active(&pats.p[set.pattern % NPAT], sl))
        sl--;
    return sl;
}
static int brk_shown(uint32_t sl)         /* (a break row on the grid: SELECT's FULL view, BREAK ROWS 16 or 32; 0.9.31,
                                          * FLEXOR: BEGINNER too) */
{
    return brk_view() > sl;
}
static int lane_ok(uint32_t sl)          /* (a lane ALGORITHM stops on: the pattern's drums, the break's rows shown) */
{
    return sl >= SL_KICK ? sl_active(&pats.p[set.pattern % NPAT], sl) : brk_shown(sl);
}
static void seq_lane_step(int32_t d)     /* ALGORITHM (and SEQ again): the next lane the pattern has, top to bottom, round
                                          * (0.9.31, FLEXOR: the whole sequencer, the break's rows shown and every bar
                                          * of every drum) */
{
    int32_t sl = SEQ_BRK ? (d > 0 ? -1 : (int32_t)NSEQL) : (int32_t)ui.seq_lane, n;
    ui.seq_brk = 0u;
    for (n = 0; n < (int32_t)NSEQL; n++) {
        sl = (sl + (d > 0 ? 1 : -1) + (int32_t)NSEQL) % (int32_t)NSEQL;
        if (lane_ok((uint32_t)sl)) {
            ui.seq_lane = (uint8_t)sl;
            return;
        }
    }
}
static void seq_view_set(uint32_t v)     /* SEQ's view (0.9.31, FLEXOR, SELECT): 0 FULL, the break's rows (SETTINGS' BREAK
                                          * ROWS: 16 or 32) and the drums; 1 DRUMS, the drums alone */
{
    set.brkv = (uint8_t)(BRKV_TAG | (v ? 0u : bk_seq() & 2u ? 1u : 2u));
    set_dirty = 1u;
    ui.seq_brk = 0u;
    if (!lane_ok(ui.seq_lane))
        seq_lane_step(1);
}
static void seq_view_turn(int32_t d)     /* SELECT on SEQ: FULL, DRUMS, SETTINGS (GP_FILL), round */
{
    uint32_t v = (brk_view() ? 0u : 1u) + (d > 0 ? 1u : 2u);
    if (v % 3u == 2u)
        global_part(GP_FILL);
    else
        seq_view_set(v % 3u);
}
static void seq_value(int i, char *t);
enum { UNDO_RANDOM = 1, UNDO_CLEAR, UNDO_LANE };   /* (UNDO_LANE + the knob) */
static void undo_mark(uint32_t what);
static uint32_t n_slices(void)
{
    const brk_break_t *b = brk_get_break();
    return b && b->nslices < 16u ? b->nslices : 16u;
}
static pat_t *pat_now(void) { return &pats.p[set.pattern % NPAT]; }

/* undo: a copy of the pattern taken before a change that wipes (SEL + ARP, SEL + REC, the drum lanes' FILL, CLEAR /
 * 4X4, COPY); a burst of the same change within 1.5 s is one. SEL + SAVE swaps it back in, and again swaps it out (redo) */
static pat_t undo_pat;
static uint8_t undo_kn[13];              /* (LOAF BREAKER: its kick notes kept apart, pats.kn; its drums' HITS) */
static uint8_t undo_steps[STEPB_N];
static uint16_t undo_dx[6];              /* (0.9.30: its drums' bars 3 and 4) */
static uint8_t undo_idx = 0xFFu, undo_what;
static uint32_t undo_t;
static void undo_mark(uint32_t what)     /* what: which change (UNDO_*), a burst of the same one is one */
{
    uint32_t now = plat_ms(), k;
    if (undo_idx != set.pattern % NPAT || what != undo_what || now - undo_t > 1500u) {
        undo_what = (uint8_t)what;
        undo_pat = *pat_now();
        undo_idx = (uint8_t)(set.pattern % NPAT);
        for (k = 0; k < 13u; k++)
            undo_kn[k] = pats.kn[undo_idx][k];
        for (k = 0; k < STEPB_N; k++)
            undo_steps[k] = steps.b[undo_idx][k];
        for (k = 0; k < 6u; k++)
            undo_dx[k] = dx.b[set.pgrp % NGRP][undo_idx][k];
    }
    undo_t = now;
}
static void undo_swap(void)
{
    pat_t t;
    char n[6] = "P";
    if (undo_idx >= NPAT) {
        app_say("UNDO", "NOTHING YET");
        return;
    }
    t = pats.p[undo_idx];
    pats.p[undo_idx] = undo_pat;
    undo_pat = t;
    {
        uint32_t k;
        for (k = 0; k < 13u; k++) {
            uint8_t b = pats.kn[undo_idx][k];
            pats.kn[undo_idx][k] = undo_kn[k];
            undo_kn[k] = b;
        }
        for (k = 0; k < STEPB_N; k++) {
            uint8_t b = steps.b[undo_idx][k];
            steps.b[undo_idx][k] = undo_steps[k];
            undo_steps[k] = b;
        }
        for (k = 0; k < 6u; k++) {
            uint16_t b = dx.b[set.pgrp % NGRP][undo_idx][k];
            dx.b[set.pgrp % NGRP][undo_idx][k] = undo_dx[k];
            undo_dx[k] = b;
        }
    }
    undo_t = 0u;                         /* (the next change takes a fresh copy) */
    if (undo_idx == set.pattern % NPAT)
        pat_apply();
    set_dirty = 1u;
    int_text((int32_t)(set.pgrp % NGRP * NPAT + undo_idx) + 1, n + 1);
    app_say("UNDO", n);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* LOAF BREAKER: SEL + SEQ copies the pattern playing; SEL + SEQ on another pattern pastes it there (all of it: the
 * break's steps, every drum and its length), once (SEL + SAVE takes it back) */
static pat_t clip_pat;
static uint8_t clip_kn[13], clip_steps[STEPB_N];
static uint16_t clip_dx[6];
static int8_t clip_from = -1;            /* the pattern copied, -1 none */
static void pat_copy_paste(void)
{
    char t[16] = "P";
    uint32_t cur = set.pattern % NPAT, all = pat_cur(), k;   /* (its slot in the group; its number in all 64) */
    if (clip_from < 0 || (uint32_t)clip_from == all) {
        clip_pat = pats.p[cur];
        for (k = 0; k < 13u; k++)
            clip_kn[k] = pats.kn[cur][k];
        for (k = 0; k < STEPB_N; k++)
            clip_steps[k] = steps.b[cur][k];
        for (k = 0; k < 6u; k++)
            clip_dx[k] = dx.b[set.pgrp % NGRP][cur][k];
        clip_from = (int8_t)all;
        int_text((int32_t)all + 1, t + 1);
        app_say("COPIED", t);
        return;
    }
    undo_mark(UNDO_CLEAR);
    pats.p[cur] = clip_pat;
    for (k = 0; k < 13u; k++)
        pats.kn[cur][k] = clip_kn[k];
    for (k = 0; k < STEPB_N; k++)
        steps.b[cur][k] = clip_steps[k];
    for (k = 0; k < 6u; k++)
        dx.b[set.pgrp % NGRP][cur][k] = clip_dx[k];
    pat_apply();
    set_dirty = 1u;
    {
        char u[4];
        int_text(clip_from + 1, t + 1);
        int_text((int32_t)all + 1, u);
        text_cat(t, " > P");
        text_cat(t, u);
    }
    clip_from = -1;
    app_say("PASTED", t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static uint32_t pat_len(void)            /* the steps the engine plays of a pattern: the break's (2 bars: 32) */
{
    const brk_break_t *b = brk_get_break();
    uint32_t n = (b ? b->bars : 2u) * BRK_STEPS_PER_BAR;
    return n < PAT_STEPS ? n : PAT_STEPS;
}
static void lane_toggle(uint32_t sl, uint32_t i)   /* sl: SL_KICK.. */
{
    uint32_t m = sl_get(pat_now(), sl) ^ 1u << i;
    sl_set(pat_now(), sl, m);
    tut_lane++;                          /* (LOAF BREAKER: the TUTORIAL's count) */
    pat_lanes_out();
    say_int(m >> i & 1u ? DRUM_NAME[sl_drum(sl)] : "OFF", (int32_t)i + 1 + 16 * (int32_t)SL_BAR[sl % NSEQL], "");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
    set_dirty = 1u;
}
/* REC: the step a key or a note lands on (the one playing, or the next past its middle) */
static uint32_t rec_at(uint32_t len, int bar)
{
    brk_status_t st;
    int32_t k;
    brk_status(&st);
    k = bar ? st.bar16 : st.step;
    if (k < 0)
        k = 0;
    return ((uint32_t)k + (st.phase > 0.5f ? 1u : 0u)) % len;
}
static int playing_now(void)
{
    brk_status_t st;
    brk_status(&st);
    return st.playing;
}
static uint8_t rec_step;                 /* LOAF BREAKER: REC stopped, step by step: the step the next key writes */
static void rec_slice(uint32_t s)
{
    int live = playing_now();
    uint32_t n = pat_len(), k = live ? rec_at(n, 0) : rec_step % n;   /* (stopped: the step shown, then the next) */
    pat_now()->st[k][0] = (uint8_t)(s + 2u);
    pat_step_out(k);
    set_dirty = 1u;
    if (!live) {
        char t[12], u[4];
        rec_step = (uint8_t)((k + 1u) % n);
        int_text((int32_t)rec_step + 1, t);
        int_text((int32_t)n, u);
        text_cat(t, " / ");
        text_cat(t, u);
        app_say("REC STEP", t);
        ui.redraw |= RG_BLOCKS;
    }
}
/* LOAF BREAKER: REC stopped, step by step: OCT+ a REST (the step left as it is, on to the next), OCT- a step back */
static void rec_move(int32_t d)
{
    char t[12], u[4];
    uint32_t n = pat_len();
    rec_step = (uint8_t)((rec_step % n + (d > 0 ? 1u : n - 1u)) % n);
    int_text((int32_t)rec_step + 1, t);
    int_text((int32_t)n, u);
    text_cat(t, " / ");
    text_cat(t, u);
    app_say(d > 0 ? "REST" : "BACK", t);
    ui.redraw |= RG_BLOCKS;
}
static void midi_slice(int s, int on)    /* a note 48..63: as the white key, recorded with REC */
{
    if (on && ui.rec)
        rec_slice((uint32_t)s);
    brk_key(s, on);
}
static void midi_drum(uint32_t d)        /* a drum note (DR_*): the hit now, and with REC into its lane */
{
    if (d == DR_KICK)
        kick_hit();
    else
        perc_hit(d - 1u);
    if (ui.rec && playing_now()) {
        brk_status_t st;
        uint32_t k, b;
        brk_status(&st);                 /* (into its bar playing) */
        k = ((uint32_t)(st.bar64 < 0 ? 0 : st.bar64) + (st.phase > 0.5f ? 1u : 0u)) % 64u;
        b = (k >> 4) % drum_bars(pat_now(), d);
        drum_bar_set(pat_now(), d, b, drum_bar(pat_now(), d, b) | 1u << (k & 15u));
        pat_lanes_out();
        set_dirty = 1u;
    }
}
/* K1-K4 on a drum lane (SEQ): FILL n hits spread evenly (Euclidean, from the downbeat), KICK FLT (LOAF BREAKER, in
 * place of ROTATE: the kick's DJ filter, HOME's K3's),
 * CLEAR (left) or four on the floor (right), LENGTH (GLOUPI BREAKS, in place of COPY: the kick 16, 32 or 64 steps, the
 * snare 16 or 32) */
static void kick_mem_turn(int32_t d);    /* (kick.c, perc.c: further on) */
static void perc_mem_turn(uint32_t v, int32_t d);
static void lane_sound_turn(int32_t d)   /* LOAF BREAKER: SEQ K2 on a drum lane: what it plays */
{
    uint32_t dr = sl_drum(ui.seq_lane);
    if (dr == DR_KICK)
        kick_mem_turn(d);
    else
        perc_mem_turn(dr - 1u, d);
}
static void lane_sound_text(char *t)
{
    uint32_t dr = sl_drum(ui.seq_lane);
    t[0] = 0;
    text_cat(t, dr == DR_KICK ? "KICK " : DRUM_NAME[dr]);
    if (dr != DR_KICK)
        text_cat(t, " ");
    int_text((int32_t)(dr == DR_KICK ? set.kslot % KICK_MEMS : *perc_slot(dr - 1u)) + 1, t + text_n(t));
}
/* LOAF BREAKER (FLEXOR, 11 Nov 2026): BEGINNER's drum lanes as EXPERT's (0.229: K1 FILL again, the BEAT presets of
 * 0.224-0.228 gone, FLEXOR: "a sequencer without them"), but no step menu (a step held: nothing) and K4 LENGTH the
 * kick's alone (0.227: 16, 32, 64; a drum lane says EXPERT) */
static void lane_knob(int i, int32_t d)
{
    static const char *const NAME[4] = {"FILL", "SOUND", "LANE", "LENGTH"};
    uint32_t m = sl_get(pat_now(), ui.seq_lane), n = 0u, k;
    char t[16];
    if (set.novice && ui.note_key) {
        easy_no();                       /* (BEGINNER: no step menu) */
        return;
    }
    if (set.novice && i == 3 && !sl_kick(ui.seq_lane)) {   /* (0.227, FLEXOR: BEGINNER's LENGTH the kick's alone, 16,
                                          * 32 or 64; the drums 16) */
        app_say("16 STEPS", "LONGER: EXPERT MODE");
        return;
    }
    if (ui.note_key && sl_notes(ui.seq_lane) && (i || !sl_tuned(ui.seq_lane))) {   /* LOAF BREAKER: a step held, K2
                                          * its HITS (1..4 in its step), K3 its CHANCE, K4 its NUDGE (later) */
        uint32_t s = ui.note_key - 1u, dr = sl_drum(ui.seq_lane), ks = SL_BAR[ui.seq_lane % NSEQL] * 16u + s;
        uint8_t *b = stepb(set.pattern, dr, ks);
        static const char *const W[4] = {"", "HITS", "COND", "NUDGE"};
        if (!i)
            return;                      /* (K1: the snare, the hat, the clap have no note) */
        if (i == 1)
            *b = (uint8_t)((*b & ~3u) | (uint32_t)clamp((int32_t)(*b & 3u) + (d > 0 ? 1 : -1), 0, 3));
        else if (i == 2)                 /* (LOAF BREAKER TEST: ALWAYS, 75 %, 50 %, 25 %, 1:2, 2:2, FILL, NOT FILL) */
            *b = (uint8_t)((*b & ~0x1Cu) | (uint32_t)clamp((int32_t)(*b >> 2 & 7u) + (d > 0 ? 1 : -1), 0, 5) << 2);   /* (0.244: FILL, NOT
                                          * FILL gone with the FILL) */
        else
            *b = (uint8_t)((*b & 0x1Fu) | (uint32_t)clamp((int32_t)(*b >> 5) + (d > 0 ? 1 : -1), 0, 7) << 5);
        ui.note_turned = 1u;
        set_dirty = 1u;
        seq_value(i, t);
        say_int(W[i & 3], (int32_t)ks + 1, "");   /* ("HITS 5": the step) */
        app_say(ui.msg[0], t);
        ui.redraw |= RG_BLOCKS | RG_FOOT;
        return;
    }
    if (i == 0 && ui.note_key && sl_tuned(ui.seq_lane)) {   /* LOAF BREAKER: a kick step held, K1: its note, -8 .. +7
                                          * semitones (that step of the lane's bar: K1..K4) */
        uint32_t s = ui.note_key - 1u, ks = SL_BAR[ui.seq_lane % NSEQL] * 16u + s;
        int32_t v = kick_note(pat_now(), ks) + (d > 0 ? 1 : -1);
        kick_note_set(pat_now(), ks, v);
        ui.note_turned = 1u;
        set_dirty = 1u;
        t[0] = 0;
        v = kick_note(pat_now(), ks);
        if (v > 0)
            text_cat(t, "+");
        int_text(v, t + (v > 0 ? 1 : 0));
        text_cat(t, " ST");
        say_int("KICK NOTE", (int32_t)ks + 1, "");
        app_say(ui.msg[0], t);
        ui.redraw |= RG_BLOCKS;
        return;
    }
    if (i == 1) {                        /* LOAF BREAKER: K2 what this lane plays (the drums' filter: HOME's K3 alone):
                                          * the kick's lanes KICK 1-10, the snare's, the hat's, the clap's their
                                          * memories, the MELODY's the pattern's instrument (a step held: its HITS) */
        lane_sound_turn(d);
        ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
        return;
    }
    if (i == 3) {                        /* GLOUPI BREAKS: LENGTH, the kick 16, 32 or 64 steps, the snare 16 or 32 */
        uint8_t *len = &PAT_LEN(pat_now());
        if (sl_kick(ui.seq_lane)) {      /* the kick: 16, 32 or 64 steps, right longer */
            uint32_t at = kick_bars(pat_now()) == 1u ? 0u : kick_bars(pat_now()) == 2u ? 1u : 2u;
            at = (uint32_t)clamp((int32_t)at + (d > 0 ? 1 : -1), 0, 2);
            *len = (uint8_t)((*len & ~(PAT_KICK16 | PAT_KICK32)) | (at == 0u ? PAT_KICK16 : at == 1u ? PAT_KICK32 : 0u));
        } else {                         /* the snare, the hat, the clap: 16, 32 or 64 (0.9.30), right longer */
            uint32_t dr = sl_drum(ui.seq_lane), at = drum_bars(pat_now(), dr) / 2u;   /* (0, 1, 2) */
            at = (uint32_t)clamp((int32_t)at + (d > 0 ? 1 : -1), 0, 2);
            *len = (uint8_t)((*len & ~(LEN_BIT[dr] | LEN64[dr])) | (at == 1u ? LEN_BIT[dr] : at == 2u ? LEN64[dr] : 0u));
        }
        pat_lanes_out();
        seq_lane_fix();
        set_dirty = 1u;
        seq_value(i, t);
        app_say(NAME[i], t);
        ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
        return;
    }
    undo_mark(UNDO_LANE + (uint32_t)i);
    if (i == 0) {
        for (k = m; k; k >>= 1)
            n += k & 1u;
        n = (uint32_t)clamp((int32_t)n + (d > 0 ? 1 : -1), 0, 16);
        for (m = 0u, k = 0u; k < 16u; k++)
            if (k * n % 16u < n)
                m |= 1u << k;
    } else                               /* CLR / 4X4: left the lane's bar cleared, right four on the floor (LOAF
                                          * BREAKER: no more 8X8, 16X16, 2-STEP, HALF-TIME after it) */
        m = d > 0 ? 0x1111u : 0u;
    sl_set(pat_now(), ui.seq_lane, m);
    pat_lanes_out();
    set_dirty = 1u;
    seq_value(i, t);
    app_say(NAME[i], i == 2 ? (m ? "4X4" : "CLEARED") : t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* LOAF BREAKER: SEQ held, the lane shown: + a white key 1..4, its bar copied onto that one (its steps' HITS, COND,
 * NUDGE and notes with it); + K1, the whole lane (every bar of its drum, or the break's 32 steps) a step later or
 * earlier, round; + REC, every drum lane of the pattern cleared, the break kept. SEL + SAVE undoes each */
static uint32_t lane_len(const pat_t *p, uint32_t sl)   /* the lane's steps: the break 32, a drum its bars' */
{
    return sl < SL_KICK ? PAT_STEPS : 16u * sl_bars(p, sl);
}
static void seq_copy_bar(uint32_t to)
{
    pat_t *p = pat_now();
    uint32_t sl = ui.seq_lane % NSEQL, from = SL_BAR[sl], s, d = sl_drum(sl);
    char t[24] = "BAR ", n[4];
    if (to * 16u >= lane_len(p, sl)) {
        app_say("COPY", "THE LANE HAS NO SUCH BAR");
        return;
    }
    if (to == from)
        return;
    undo_mark(UNDO_LANE + 4u);
    for (s = 0; s < 16u; s++) {
        uint32_t a = from * 16u + s, b = to * 16u + s, k;
        if (sl < SL_KICK) {
            for (k = 0; k < 4u; k++)
                p->st[b][k] = p->st[a][k];
            pat_step_out(b);
            continue;
        }
        *stepb(set.pattern, d, b) = *stepb(set.pattern, d, a);
        if (d == DR_KICK)
            kick_note_set(p, b, kick_note(p, a));
    }
    if (sl >= SL_KICK) {
        drum_bar_set(p, d, to, drum_bar(p, d, from));
        pat_lanes_out();
    }
    set_dirty = 1u;
    int_text((int32_t)from + 1, n);
    text_cat(t, n);
    text_cat(t, " > BAR ");
    int_text((int32_t)to + 1, n);
    text_cat(t, n);
    app_say("COPY", t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void seq_rotate(int32_t d)
{
    pat_t *p = pat_now();
    uint32_t sl = ui.seq_lane % NSEQL, n = lane_len(p, sl), dr = sl_drum(sl), s, k;
    uint8_t on[64], sb[64], br[PAT_STEPS][4];
    int8_t nt[64];
    undo_mark(UNDO_LANE + 5u);
    for (s = 0; s < n; s++) {            /* (the lane as it is) */
        if (sl < SL_KICK) {
            for (k = 0; k < 4u; k++)
                br[s][k] = p->st[s][k];
            continue;
        }
        on[s] = (uint8_t)(drum_bar(p, dr, s >> 4) >> (s & 15u) & 1u);
        sb[s] = *stepb(set.pattern, dr, s);
        nt[s] = (int8_t)(dr == DR_KICK ? kick_note(p, s) : 0);
    }
    for (s = 0; s < n; s++) {            /* (each step takes the one before it, or after it) */
        uint32_t f = (s + (d > 0 ? n - 1u : 1u)) % n;
        if (sl < SL_KICK) {
            for (k = 0; k < 4u; k++)
                p->st[s][k] = br[f][k];
            pat_step_out(s);
            continue;
        }
        *stepb(set.pattern, dr, s) = sb[f];
        if (dr == DR_KICK)
            kick_note_set(p, s, nt[f]);
    }
    if (sl >= SL_KICK) {
        for (k = 0; k < n / 16u; k++) {
            uint32_t m = 0u;
            for (s = 0; s < 16u; s++)
                m |= (uint32_t)on[(k * 16u + s + (d > 0 ? n - 1u : 1u)) % n] << s;
            drum_bar_set(p, dr, k, m);
        }
        pat_lanes_out();
    }
    set_dirty = 1u;
    app_say("SHIFT", d > 0 ? "A STEP LATER" : "A STEP EARLIER");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void seq_clear_drums(void)
{
    pat_t *p = pat_now();
    uint32_t d, b;
    undo_mark(UNDO_LANE + 6u);
    for (d = 0; d < NDRUMS; d++)
        for (b = 0; b < 4u; b++)
            drum_bar_set(p, d, b, 0u);
    pat_lanes_out();
    set_dirty = 1u;
    app_say("DRUMS", "CLEARED (THE BREAK KEPT)");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
static void seq_key(uint32_t i)          /* a white key on SEQ: the step to edit, or a drum step on and off */
{
    char t[8];
    seq_lane_fix();
    if (ui.seq_lane >= SL_KICK) {
        lane_toggle(ui.seq_lane, i);
        return;
    }
    ui.seq_step = (uint8_t)(ui.seq_lane * 16u + i);
    seq_value(0, t);
    say_int("STEP", (int32_t)ui.seq_step + 1, "");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* SEL + REC: 0 the whole pattern (elsewhere), 1 on SEQ the lane shown; LOAF BREAKER: 2 REC held on SEQ, the break's
 * steps, both halves (1-16, 17-32), the drums kept */
static void seq_clear(int mode)
{
    pat_t *p = pat_now();
    uint32_t k;
    undo_mark(UNDO_CLEAR);
    if (mode != 1 || ui.seq_lane < SL_KICK)
        for (k = 0; k < PAT_STEPS; k++)
            if (mode != 1 || k / 16u == ui.seq_lane) {
                p->st[k][0] = 1u;
                p->st[k][1] = 0u;
                p->st[k][2] &= 0xF8u;    /* (bits 3-7: the MELODY's, 0.200-0.208) */
            }
    if (!mode) {
        for (k = SL_KICK; k < NSEQL; k++)
            sl_set(p, k, 0u);
        for (k = 0; k < PAT_STEPS; k++)  /* (LOAF BREAKER: the old MELODY's bits too, and the kick's notes) */
            p->st[k][2] &= 0x07u;
        for (k = 0; k < 64u; k++)
            kick_note_set(p, k, 0);
        for (k = 0; k < STEPB_N; k++)    /* (and every drum step's own settings) */
            steps.b[set.pattern % NPAT][k] = 0u;
    }
    else if (mode == 1 && ui.seq_lane >= SL_KICK)
        sl_set(p, ui.seq_lane, 0u);
    pat_apply();
    set_dirty = 1u;
    app_say("CLEARED", mode == 1 ? SEQ_LANE_NAME[ui.seq_lane] : mode ? "BREAK 1-32" : "PATTERN");
    ui.redraw |= RG_BLOCKS | RG_FOOT;
}
/* a note's name: "C3", "F#4" (0: C3; kick.c's KICK LAB NOTE) */
static void note_name(int32_t n, char *t)   /* "C3", "F#4" (0: C3) */
{
    static const char *const NN[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int32_t o = (n + 120) / 12 - 10;
    t[0] = 0;
    text_cat(t, NN[(n + 120) % 12]);
    int_text(3 + o, t + text_n(t));
}
static void seq_value(int i, char *t)    /* the step edited: SLICE, PITCH, HITS, REVERSE */
{
    const uint8_t *b = pat_now()->st[ui.seq_step % PAT_STEPS];
    t[0] = 0;
    if (SEQ_BRK) {                       /* (LOAF BREAKER, 0.234: BREAK, K4 its rows shown) */
        if (i == 3)
            text_cat(t, brk_view() == 0u ? "HIDDEN" : brk_view() == 1u ? "16 STEPS" : "32 STEPS");
        return;
    }
    if (ui.seq_lane >= SL_KICK && ui.note_key) {   /* LOAF BREAKER: a step held: its NOTE, HITS, CHANCE, NUDGE */
        uint32_t ks = SL_BAR[ui.seq_lane % NSEQL] * 16u + ui.note_key - 1u, dr = sl_drum(ui.seq_lane);
        uint32_t sb = *stepb(set.pattern, dr, ks);
        if (i == 0) {
            if (!sl_tuned(ui.seq_lane))
                text_cat(t, "-");
            else {
                int32_t v = kick_note(pat_now(), ks);
                if (v > 0)
                    text_cat(t, "+");
                int_text(v, t + (v > 0 ? 1 : 0));
                text_cat(t, " ST");
            }
        } else if (i == 1) {
            int_text((int32_t)(sb & 3u) + 1, t);
            text_cat(t, (sb & 3u) ? " HITS" : " HIT");
        } else if (i == 2) {             /* LOAF BREAKER TEST: CHANCE and the trig conditions */
            static const char *const C[8] = {"ALWAYS", "75%", "50%", "25%", "1:2", "2:2", "FILL", "NOT FILL"};
            text_cat(t, C[(sb >> 2 & 7u) >= 6u ? 0u : sb >> 2 & 7u]);   /* (0.244: FILL's two, ALWAYS now) */
        } else if (!(sb >> 5))
            text_cat(t, "ON TIME");
        else {
            text_cat(t, "+");
            int_text((int32_t)(sb >> 5), t + 1);
            text_cat(t, "/16");
        }
        return;
    }
    if (ui.seq_lane >= SL_KICK) {        /* a drum lane: FILL (its hits), KICK FLT, CLEAR / 4X4, LENGTH */
        uint32_t m = sl_get(pat_now(), ui.seq_lane), n = 0u;
        while (m) {
            n += m & 1u;
            m >>= 1;
        }
        if (i == 0) {
            int_text((int32_t)n, t);
            text_cat(t, " HITS");
        } else if (i == 1)
            lane_sound_text(t);          /* (LOAF BREAKER: what the lane plays) */
        else if (i == 2)
            text_cat(t, "L CLR R 4X4");
        else {
            int_text((int32_t)(16u * sl_bars(pat_now(), ui.seq_lane)), t);
            text_cat(t, " STEPS");
        }
        return;
    }
    if (i == 0) {
        if (b[0] < 2u)
            text_cat(t, b[0] ? "LOOP" : "OFF");
        else
            int_text(b[0] - 1, t);
    } else if (i == 1) {
        int_text((int8_t)b[1], t);
        text_cat(t, " ST");
    } else if (i == 2) {
        text_cat(t, "X");
        int_text(((b[2] >> 1) & 3u) + 1, t + 1);
    } else
        text_cat(t, b[2] & 1u ? "REVERSE" : "FORWARD");
}
static void seq_turn(int i, int32_t d)
{
    uint8_t *b = pat_now()->st[ui.seq_step % PAT_STEPS];
    char t[12];
    seq_lane_fix();
    if (SEQ_BRK) {                       /* (LOAF BREAKER, 0.234: BREAK: K4 the break's rows shown, HIDDEN, 16, 32) */
        if (i == 3) {
            set.brkv = (uint8_t)(BRKV_TAG | (uint32_t)clamp((int32_t)brk_view() + (d > 0 ? 1 : -1), 0, 2));
            set_dirty = 1u;
            seq_value(i, t);
            app_say("BREAK STEPS", t);
            ui.redraw |= RG_BLOCKS | RG_FOOT;
        } else
            app_say("BREAK", "K4: ITS STEPS SHOWN");
        return;
    }
    if (ui.seq_lane >= SL_KICK) {
        lane_knob(i, d);
        return;
    }
    if (i == 0)                          /* OFF, LOOP (follow the break), slices 1.. */
        b[0] = (uint8_t)clamp((int32_t)b[0] + d, 0, (int32_t)(n_slices() + 1u));
    else if (i == 1)
        b[1] = (uint8_t)(int8_t)clamp((int8_t)b[1] + d, -12, 12);
    else if (i == 2)
        b[2] = (uint8_t)((b[2] & ~6u) | (uint32_t)clamp((int32_t)((b[2] >> 1) & 3u) + (d > 0 ? 1 : -1), 0, 3) << 1);
    else
        b[2] = (uint8_t)((b[2] & ~1u) | (d > 0 ? 1u : 0u));
    pat_step_out(ui.seq_step % PAT_STEPS);
    set_dirty = 1u;
    seq_value(i, t);
    say_int("STEP", (int32_t)ui.seq_step + 1, "");
    app_say(ui.msg[0], t);
    ui.redraw |= RG_BLOCKS | RG_FOOT;    /* (the knobs' values too: they show the step) */
}
static const char *grp_name(uint32_t g)  /* LOAF BREAKER: a group of 16 as the screen says it */
{
    static const char *const N[NGRP] = {"P1-16", "P17-32", "P33-48", "P49-64"};
    return N[g % NGRP];
}
static int grp_blocked(uint32_t g)       /* LOAF BREAKER: pattern g (0..63) can't come in now: another group, this
                                          * one's changes not saved, playing (said) */
{
    if (g / NPAT % NGRP == set.pgrp || !grp_changed() || !playing_now())
        return 0;
    app_say("SAVE OR STOP", grp_name(set.pgrp));
    return 1;
}
static int pat_go(uint32_t g)            /* LOAF BREAKER: pattern g (0..63) the one playing (its group in first: this
                                          * one's changes saved before, stopped); 0 refused */
{
    uint32_t grp = g / NPAT % NGRP;
    if (grp != set.pgrp) {
        if (grp_blocked(g))
            return 0;
        if (grp_changed() && !grp_save()) {
            app_say("SAVE", "FAILED");
            return 0;
        }
        grp_load(grp);
        undo_idx = 0xFFu;                /* (an undo was the other group's) */
    }
    set.pattern = (uint8_t)(g % NPAT);
    return 1;
}
static void pattern_pick(int32_t d)      /* PRESETS: the next or previous pattern (playing: at the loop's end) */
{
    int32_t cur = pat_next >= 0 ? pat_next : (int32_t)pat_cur();
    int32_t want = (cur + (d > 0 ? 1 : (int32_t)NPAT_ALL - 1)) % (int32_t)NPAT_ALL;
    if (grp_blocked((uint32_t)want))
        return;
    if (playing_now())
        pat_next = (int8_t)want;
    else {
        if (!pat_go((uint32_t)want))
            return;
        pat_apply();
        set_dirty = 1u;
    }
    say_int(playing_now() ? "NEXT PATTERN" : "PATTERN", want + 1, "");
    ui.redraw |= RG_HEAD | RG_BLOCKS | RG_FOOT;
}
static uint32_t track_keys_lit(void);
static int8_t lit_head = -1;             /* (0.9.57: the step playing on a lane's page, 0..15, -1 none: leds() shows it
                                          * at the middle level; it was the step's light turned over) */
static uint32_t keys_lit(const brk_status_t *st)   /* the white keys' lights on a lane's page (ENV: its own) */
{
    lit_head = -1;
    if (ui.page == PG_TRACK)
        return track_keys_lit();
    if (ui.page == PG_OVL)               /* (0.9.27, FLEXOR: the OVERLAY page, its effects on lit: held, latched, the
                                          * ARP key's) */
        return ui.opage ? 0u : ovl_held | ovl_latched | ovl_pad_bits();
    if (ui.page == PG_GLOBAL)            /* (LOAF BREAKER: GLO EFFECTS: the effects on) */
        return ui.gpage == 3u ? set.sfx_on : 0u;
    uint32_t m = 0u, i;
    if (ui.seq_lane >= SL_KICK) {
        int now = sl_now(st, ui.seq_lane);
        m = sl_get(pat_now(), ui.seq_lane);
        if (now >= 0)
            lit_head = (int8_t)now;
        return m;
    }
    for (i = 0; i < 16u; i++)            /* SEQ's break steps: the ones with a slice of their own */
        if (pat_now()->st[ui.seq_lane * 16u + i][0] >= 2u)
            m |= 1u << i;
    if (st->playing && st->step >= 0 && (uint32_t)st->step / 16u == ui.seq_lane)
        lit_head = (int8_t)(st->step % 16);
    return m;
}
/* LOAF BREAKER: the SEQ page, everything at once. Under the tabs (y 44..191, drawn in three bands), a row of 16 steps
 * each: the break's half the keys edit, when they edit it (each step its slice, LOOP a dot, off a dash; the one edited
 * outlined; gone while a drum lane is picked), then every drum
 * over the length it has (the kick's 1, 2 or 4 bars, the snare's, hat's and clap's 1 or 2), named by its letters
 * ("K", "SN"; its other bars "2", "3", "4"). The rows share the height: fewer rows, bigger steps. The step playing
 * white; the lane the keys edit named lit, a line under it */
static uint32_t seq_rows(uint8_t *row)   /* the rows, top to bottom (SL_*); their count */
{
    uint32_t sl, n = 0u;
    for (sl = 0; sl < NSEQL; sl++)       /* (the break's half: only while the keys edit it; LOAF BREAKER, 0.232, FLEXOR:
                                          * BEGINNER, its two halves always on top, the function in sight) */
        if (sl >= SL_KICK ? sl_active(pat_now(), sl) : brk_shown(sl) || (sl == ui.seq_lane && !SEQ_BRK))
            row[n++] = (uint8_t)sl;
    return n;
}
static void seq_picture(const brk_status_t *st, int32_t top)
{
    static const char *const TAG[NDRUMS] = {"K", "SN", "HH", "CP"};
    const pat_t *p = pat_now();
    uint8_t row[NSEQL];
    uint32_t i, r, n = seq_rows(row), nb = 0u;
    int32_t y0, pitch, bh;
    char t[8];
    while (nb < n && row[nb] < SL_KICK)  /* (the break's rows on top: one, or BEGINNER's two) */
        nb++;
    bh = SQ_H / (int32_t)(n ? n : 1u);   /* (0.242: the break's rows as tall as the grid's average, its numbers in) */
    bh = bh < SQ_BRK_MIN ? SQ_BRK_MIN : bh > SQ_BRK_MAX ? SQ_BRK_MAX : bh;
    y0 = bh * (int32_t)nb;
    pitch = (SQ_H - y0) / (int32_t)(n - nb ? n - nb : 1u);
    cv_oy = -top;
    for (r = 0; r < n; r++) {
        uint32_t sl = row[r], brk = sl < SL_KICK, m = brk ? 0u : sl_get(p, sl);
        int32_t y = brk ? bh * (int32_t)r : y0 + (int32_t)(r - nb) * pitch, at = brk ? -1 : sl_now(st, sl);
        int32_t ch = brk ? bh - 2 : pitch > 4 ? pitch - 2 : pitch;   /* (the break's half: tall enough for its
                                          * numbers) */
        int picked = SEQ_BRK ? brk : sl == ui.seq_lane;   /* (on BREAK: the break's rows named lit) */
        t[0] = 0;                        /* the row's name: the drum and its length on its first bar, else the bar */
        if (brk)
            text_cat(t, sl ? "BRK2" : "BRK1");
        else if (SL_BAR[sl]) {
            t[0] = (char)('1' + SL_BAR[sl]);
            t[1] = 0;
        } else
            text_cat(t, TAG[sl_drum(sl)]);   /* (LOAF BREAKER: the letters alone, the length the grid shows) */
        cv_text(SQ_X - 3 - text_w(&FONT_XS, t), y + (ch - 8) / 2, &FONT_XS, t, picked ? C_HI : brk || !SL_BAR[sl] ? C_GRAY
                : C_DIM);
        for (i = 0; i < 16u; i++) {
            int32_t x = SQ_X + (int32_t)(i * (240u - SQ_X) / 16u), w = SQ_X + (int32_t)((i + 1u) * (240u - SQ_X) / 16u) - x - 2;
            uint16_t c;
            if (brk) {                   /* a step of the break: its slice */
                uint32_t k = sl * 16u + i, v = p->st[k][0];
                int now = st->playing && st->step == (int32_t)k, sel = ui.seq_lane < SL_KICK && !SEQ_BRK && k == ui.seq_step;
                cv_rect(x, y, w, ch, sel ? C_WHITE : now ? C_BLOCK_HI : (i & 3u) ? C_BLOCK : C_LINE);
                if (sel && ch > 4)
                    cv_rect(x + 1, y + 1, w - 2, ch - 2, now ? C_BLOCK_HI : C_BLOCK);
                t[0] = 0;
                if (v >= 2u)
                    int_text((int32_t)v - 1, t);
                else
                    text_cat(t, v ? "." : "-");
                if (ch >= 9)             /* (0.242: centred, clear of the mark under it) */
                    cv_text(x + (w - text_w(&FONT_XS, t)) / 2, y + (ch - 10) / 2, &FONT_XS, t,
                            now ? C_BLACK : v >= 2u ? C_HI : C_GRAY);
                else if (v >= 2u)        /* (too small for its number: a mark) */
                    cv_rect(x + 1, y + ch / 2, w - 2, 1, now ? C_BLACK : C_HI);
                if (p->st[k][2] & 7u && ch >= 6)   /* reversed or ratcheted: a mark */
                    cv_rect(x + 2, y + ch - 2, w - 4, 1, now ? C_BLACK : C_AMB);
                continue;
            }
            c = at == (int32_t)i ? C_WHITE : m >> i & 1u ? (picked ? C_BLOCK_HI : C_AMB) : (i & 3u) ? C_BLOCK : C_LINE;
            cv_rect(x, y, w, ch, c);
            if (m >> i & 1u && ch > 4) { /* (LOAF BREAKER: the step's own: its HITS split it, a CHANCE dims its
                                          * lower half, a NUDGE marks how late) */
                uint32_t sb = *stepb(set.pattern, sl_drum(sl), SL_BAR[sl % NSEQL] * 16u + i), hn = (sb & 3u) + 1u, j;
                for (j = 1; j < hn; j++)
                    cv_rect(x + w * (int32_t)j / (int32_t)hn, y + 1, 1, ch - 2, C_BLACK);
                if (sb >> 2 & 7u)
                    cv_rect(x + 1, y + ch / 2, w - 2, ch - ch / 2 - 1, C_BLOCK);
                if (sb >> 5)
                    cv_rect(x + (w - 3) * (int32_t)(sb >> 5) / 8, y + ch - 2, 3, 2, C_WHITE);
            }
            if (at == (int32_t)i && ch > 6)
                cv_rect(x + 2, y + 2, w - 4, ch - 4, m >> i & 1u ? C_BLOCK_HI : C_BLOCK);
            if (ch > 5 && sl_kick(sl) && kick_note(p, SL_BAR[sl] * 16u + i))
                                          /* (a step with a note: a white line on top) */
                cv_rect(x + 1, y, w - 2, 2, C_WHITE);
        }
        if (picked && !brk && pitch - ch >= 2)   /* the lane the keys edit: a line under it */
            cv_rect(SQ_X, y + ch, 240 - SQ_X - 2, 1, C_HI);
    }
    cv_oy = 0;
}
static void draw_seq(const brk_status_t *st)
{
    int32_t top;
    seq_lane_fix();
    for (top = 0; top < SQ_H; top += 60) {   /* (the canvas holds 60 rows) */
        int32_t h = SQ_H - top < 60 ? SQ_H - top : 60;
        cv_begin(240, (uint32_t)h, C_BLACK);
        seq_picture(st, top);
        cv_blit(0, (uint32_t)(44 + top));
    }
}
