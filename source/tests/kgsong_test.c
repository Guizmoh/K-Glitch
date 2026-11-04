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
    printf("kgsong_test: %s\n", fails ? "FAILED" : "passed");
    return fails != 0;
}
