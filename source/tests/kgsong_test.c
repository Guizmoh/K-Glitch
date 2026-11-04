/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH (firmware/src/kg_song.c): each section keeps its own effects. SAVE writes the effect lane, INTENSITY, the
 * slices and K-GLITCH's settings with the section; NEW clears them (and lets the recorded loop go); LOAD brings back
 * what was saved, a section saved without them comes back with none; the other sections' records stay; ERASE drops
 * the section's. Exit status: failed checks. */
#include "editor_host.c"
static int fails;
static void check(int c, const char *what) { printf("kgsong: %-80s %s\n", what, c ? "ok" : "FAIL"); fails += !c; }
static void boot(void)
{
    uint32_t i;
    persist_boot();
    settings_init();
    panel_init();
    for (i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    for (i = 0; i < NTRK; i++) {
        track_defaults(&trk[i]);
        if (i < NPART) { set_engine_of(&trk[i], TRK_DEF[i][0]); apply_preset_to(&trk[i], TRK_DEF[i][1]); trk[i].engine = trk[i].eng_req; }
    }
    layers_init();
    go_home();
}
static void song_a(void)
{
    kg.seq[3] = 5, kg.seq2[7] = 2, kg.prob[3] = 60, kd_int[4] = 90, kl.map[1] = 7;
    song.g[G_KCHAOS] = 77, song.g[G_KBANK] = 2, song.g[G_KFTYPE] = 3, song.g[G_KTR1] = 40;
}
static int is_a(void)
{
    return kg.seq[3] == 5 && kg.seq2[7] == 2 && kg.prob[3] == 60 && kd_int[4] == 90 && kl.map[1] == 7 &&
           song.g[G_KCHAOS] == 77 && song.g[G_KBANK] == 2 && song.g[G_KFTYPE] == 3 && song.g[G_KTR1] == 40;
}
static int is_clear(void)
{
    uint32_t i, ok = 1;
    for (i = 0; i < KG_STEPS; i++) ok &= kg.seq[i] == -1 && kg.seq2[i] == -1;
    for (i = 0; i < DD_N; i++) ok &= kd_int[i] == 50;
    for (i = 0; i < KL_NS; i++) ok &= kl.map[i] == i;
    return ok && song.g[G_KCHAOS] == GP[G_KCHAOS].def && song.g[G_KFTYPE] == GP[G_KFTYPE].def && kl.st == KLS_EMPTY;
}
int main(void)
{
    boot();
    check(flash_ok, "the flash is there");
    song_a();
    transport_req = 0, song.playing = 0;
    project_save(0);
    kl.st = KLS_HAS;                                   /* a loop recorded */
    project_new();
    check(is_clear(), "NEW: no effect lane, INTENSITY, slices, settings or loop from the last song");
    transport_req = 0, song.playing = 0;
    project_save(1);                                   /* a section with no effects */
    transport_req = 0, song.playing = 0;
    project_load(0);
    check(is_a(), "LOAD A: its effects, automation, slices and settings back");
    transport_req = 0, song.playing = 0;
    project_load(1);
    check(is_clear(), "LOAD B: saved with none, none");
    boot();                                            /* power off, on: from the flash */
    transport_req = 0, song.playing = 0;
    project_load(0);
    check(is_a(), "after a power-on, LOAD A: its effects back (from the flash)");
    kg.seq[3] = 9;
    transport_req = 0, song.playing = 0;
    project_save(2);
    transport_req = 0, song.playing = 0;
    project_load(0);
    check(is_a(), "saving C leaves A's record as it was");
    transport_req = 0, song.playing = 0;
    project_erase(0);
    transport_req = 0, song.playing = 0;
    project_load(2);
    check(kg.seq[3] == 9, "ERASE A leaves C's");
    /* 16 songs: E..P in USR3 / USR4's flash; each with its effects and its name; USR3, USR4 hold no samples */
    {
        uint32_t k, ok = 1;
        for (k = 4; k < NSONG; k++) {
            transport_req = 0, song.playing = 0;
            project_new();
            transport_req = 0;
            song.g[G_BPM] = (int16_t)(80 + k);
            kg.seq[k] = (int8_t)k;
            str_cpy(song_name, "SONG", sizeof song_name);
            song_name[4] = (char)('A' + k), song_name[5] = 0;
            project_save(k);
            ok &= project_used(k);
        }
        check(ok, "SAVE: songs E..P (USR3 and USR4's flash)");
        for (k = 4; k < NSONG; k++) {
            transport_req = 0, song.playing = 0;
            project_new();
            transport_req = 0;
            project_load(k);
            ok &= song.g[G_BPM] == (int16_t)(80 + k) && kg.seq[k] == (int8_t)k && song_name[4] == (char)('A' + k) &&
                  !strcmp(song_name_of(k), song_name);
        }
        check(ok, "LOAD E..P: each its tempo, its effects, its name");
        transport_req = 0, song.playing = 0;
        project_load(2);
        check(kg.seq[3] == 9 && kg.seq[5] != 5, "C after them: its own (A..D and E..P apart)");
        check(usr_nz[2] == 0 && usr_nz[3] == 0, "USR3, USR4: no samples (songs there)");
        transport_req = 0;
        project_erase(9);
        check(!project_used(9) && project_used(10), "ERASE J: J empty, K kept");
        boot();
        transport_req = 0, song.playing = 0;
        project_load(15);
        check(kg.seq[15] == 15 && !strcmp(song_name, "SONGP"), "after a power-on, LOAD P (USR4's flash): back");
    }
    /* UNDO (0.9.4): the effect lane and its automation, the parameter locks, and what it took back on the screen */
    {
        track_t *t = &trk[0];
        uint32_t k;
        transport_req = 0, song.playing = 0;
        project_new();
        transport_req = 0;
        memset(&uh, 0, sizeof uh); undo.valid = undo.open = 0;
        kgu_sync();
        kg.seq[2] = 4, kg.seq[6] = 7, kg.fnote[6] = 60, kg.fmode[6] = 3;
        kgu_commit();
        kg.seq[10] = 1;
        check(undo_swap(0) && kg.seq[10] == -1 && kg.seq[2] == 4 && !strcmp(undo_what, "UNDO FX SEQ: 1 STEP"),
              "UNDO: the last effect step back, the screen says FX SEQ: 1 STEP");
        check(undo_swap(0) && kg.seq[2] == -1 && kg.seq[6] == -1 && kg.fnote[6] == 0, "UNDO again: the effects and the filter automation before");
        check(undo_swap(1) && kg.seq[2] == 4 && kg.fnote[6] == 60 && !strncmp(undo_what, "REDO FX SEQ: 2 STEPS", 20), "REDO: back");
        undo_mark(t, (undo_sess += 4u) | 3u);
        for (k = 0; k < NLOCK; k++) if (t->lock[k].step == LOCK_FREE) break;
        t->lock[k].step = 3, t->lock[k].param = P_PAN, t->lock[k].val = 99;
        check(undo_swap(0) && t->lock[k].step == LOCK_FREE && !strcmp(undo_what, "UNDO T1 + LOCKS"), "UNDO: a parameter lock (automation) taken back: T1 + LOCKS");
        check(undo_swap(1) && t->lock[k].step == 3 && t->lock[k].val == 99, "REDO: the lock again");
    }
    /* SEQ held + PRESETS on K-GLITCH's pages: the tools on the effect sequence (COPY, CLEAR, PASTE, RANDOM, REVERSE), each one undo */
    {
        uint32_t i, n = 0;
        for (i = 0; i < NPAGES; i++) if (!strcmp(PAGES[i].title, "FX SEQ")) break;
        ui.home = 0, ui.page = (uint8_t)i;
        check(seq_tool_fx(), "SEQ tools on FX > FX SEQ: the effect sequence");
        kg_clear(); kgu_sync(); memset(&uh, 0, sizeof uh);
        kg.seq[0] = 3, kg.seq[5] = 8, kg.fnote[5] = 70, kg.fmode[5] = 2, song.g[G_KSEQ] = 1;
        fx_tool_do(ST_COPY);
        fx_tool_do(ST_CLEAR);
        check(kg.seq[0] == -1 && kg.seq[5] == -1 && kg.fnote[5] == 0, "FX SEQ CLEAR: empty, its automation too");
        fx_tool_do(ST_PASTE);
        check(kg.seq[0] == 3 && kg.seq[5] == 8 && kg.fnote[5] == 70, "FX SEQ PASTE: the copy back, with its automation");
        fx_tool_do(ST_REVERSE);
        check(kg.seq[15] == 3 && kg.seq[10] == 8, "FX SEQ REVERSE: the 16 steps backwards");
        song.g[G_KDENS] = 100;
        fx_tool_do(ST_RANDOM);
        for (i = 0; i < 16u; i++) n += kg.seq[i] >= 0;
        check(n > 4u, "FX SEQ RANDOM: a new sequence");
        check(undo_swap(0) && kg.seq[15] == 3 && undo_swap(0) && kg.seq[0] == 3 && undo_swap(0) && kg.seq[0] == -1,
              "each tool one undo (RANDOM, REVERSE, PASTE back in turn)");
        ui.home = 1;
    }
    printf("kgsong_test: %s\n", fails ? "FAILED" : "passed");
    return fails != 0;
}
