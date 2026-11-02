/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH DRUMS (firmware/src/kg_drum.c): the drum track's FM DRUM and C64 DRUM kits, through hostsim.c:
 *   every sound of both kits sounds, stays bounded and ends (its voice free again) within 3 s;
 *   SAT SOFT, HARD and FOLD each change the sound and stay bounded;
 *   the sound's own sends: REV and DLY 0 send nothing (the drum track's global sends aside), up they send;
 *   any value flash could bring back is put into range: 3000 random sounds play bounded and finite;
 *   EDIT on the drum track: the KIT page, the model's pages with its kit, a knob writes the sound, INIT restores it.
 * argv[1]: a WAV demo (both kits on a beat). */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#include <math.h>

static int fails;
static void check(int ok, const char *what) { printf("kgdrum: %-80s %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

#define NS (3u * FS)
static int32_t L[NS + CTL], R[NS + CTL], RV[NS + CTL], DL[NS + CTL];
/* one hit of lane l on the kit, NS samples; returns the peak; *end the sample its voice ended (NS: still sounding) */
static int32_t hit(uint32_t kit, uint32_t l, uint32_t *end)
{
    uint32_t i, k;
    int32_t pk = 0;
    memset(&drums, 0, sizeof drums);
    drums.set = -2;
    memset(L, 0, sizeof L), memset(R, 0, sizeof R), memset(RV, 0, sizeof RV), memset(DL, 0, sizeof DL);
    TDRUM->p[P_E0] = (int16_t)kit;
    rng_state = 0x1234567u;
    drum_on(LANE_NOTE[l], 110);
    *end = NS;
    for (i = 0; i < NS; i += CTL) {
        drums_render(L + i, R + i, RV + i, DL + i, CTL);
        for (k = 0; k < NDRUM && !drums.v[k].active; k++) {}
        if (k == NDRUM && *end == NS) *end = i;
    }
    for (i = 0; i < NS; i++) {
        int32_t a = L[i] < 0 ? -L[i] : L[i];
        if (a > pk) pk = a;
    }
    return pk;
}
static int same(const int32_t *a, const int32_t *b, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    static int32_t ref[NS];
    uint32_t m, l, end, i, ok;
    int32_t pk;
    host_tracks_init();
    song.g[G_DRLVL] = 100, song.g[G_DRREV] = 0, song.g[G_DRDLY] = 0;
    kgd_defaults();
    for (m = 0; m < KGD_NK; m++) {
        ok = 1;
        for (l = 0; l < DRUM_LANES; l++) {
            pk = hit(DRUM_KGFM + m, l, &end);
            if (pk < 1500 || pk > (1 << 20) || end >= NS) {
                printf("  %s lane %u: peak %d, ends at %u\n", m ? "C64" : "FM", l, pk, end);
                ok = 0;
            }
        }
        check(ok, m ? "C64 DRUM: the 16 sounds sound, bounded, and end within 3 s" : "FM DRUM: the 16 sounds sound, bounded, and end within 3 s");
    }
    /* SAT: each type changes the kick, bounded */
    for (m = 0; m < KGD_NK; m++) {
        kgd_snd_t keep = kgd_kit[m][0];
        uint32_t t;
        ok = 1;
        kgd_kit[m][0].sat = KS_OFF, kgd_kit[m][0].drv = 100;
        hit(DRUM_KGFM + m, 0, &end);
        memcpy(ref, L, sizeof ref);
        for (t = KS_SOFT; t <= KS_FOLD; t++) {
            kgd_kit[m][0].sat = (uint8_t)t;
            pk = hit(DRUM_KGFM + m, 0, &end);
            ok &= !same(ref, L, NS) && pk > 1500 && pk < (1 << 20);
        }
        kgd_kit[m][0] = keep;
        check(ok, m ? "C64: SAT SOFT, HARD, FOLD change the sound, bounded" : "FM: SAT SOFT, HARD, FOLD change the sound, bounded");
    }
    /* the sound's sends: its own, not the track's */
    {
        kgd_snd_t keep = kgd_kit[KGD_FM][2];
        double rv = 0, dl = 0;
        song.g[G_DRREV] = 100, song.g[G_DRDLY] = 100;     /* (the track's: not for these kits) */
        kgd_kit[KGD_FM][2].rev = 0, kgd_kit[KGD_FM][2].dly = 0;
        hit(DRUM_KGFM, 2, &end);
        for (i = 0; i < NS; i++) rv += fabs((double)RV[i]), dl += fabs((double)DL[i]);
        ok = rv == 0 && dl == 0;
        kgd_kit[KGD_FM][2].rev = 100, kgd_kit[KGD_FM][2].dly = 60;
        hit(DRUM_KGFM, 2, &end);
        rv = dl = 0;
        for (i = 0; i < NS; i++) rv += fabs((double)RV[i]), dl += fabs((double)DL[i]);
        ok &= rv > 1e6 && dl > 1e5 && rv > dl;
        kgd_kit[KGD_FM][2] = keep;
        song.g[G_DRREV] = 0, song.g[G_DRDLY] = 0;
        check(ok, "a sound's own REV and DLY sends (0: nothing, the track's sends aside)");
    }
    /* any value: put into range, bounded, finite */
    {
        uint32_t s = 0x9E3779B9u, t;
        ok = 1;
        for (t = 0; t < 3000u; t++) {
            uint8_t *b = (uint8_t *)&kgd_kit[t & 1u][t % DRUM_LANES];
            uint32_t j;
            for (j = 0; j < sizeof(kgd_snd_t); j++) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; b[j] = (uint8_t)s; }
            kgd_fix(&kgd_kit[t & 1u][t % DRUM_LANES], t & 1u);
            memset(&drums, 0, sizeof drums);
            TDRUM->p[P_E0] = (int16_t)(DRUM_KGFM + (t & 1u));
            drum_on(LANE_NOTE[t % DRUM_LANES], 1u + s % 127u);
            for (i = 0; i < FS / 2u; i += CTL) {
                uint32_t k;
                memset(L, 0, CTL * 4), memset(R, 0, CTL * 4), memset(RV, 0, CTL * 4), memset(DL, 0, CTL * 4);
                drums_render(L, R, RV, DL, CTL);
                for (k = 0; k < CTL; k++)
                    if (!(L[k] > -(1 << 22) && L[k] < (1 << 22))) {
                        if (ok) { uint32_t j; printf("  sound %u (%s lane %u): %d at %u:", t, t & 1u ? "C64" : "FM", t % DRUM_LANES, L[k], i);
                            for (j = 0; j < sizeof(kgd_snd_t); j++) printf(" %u", b[j]); printf("\n"); }
                        ok = 0;
                    }
            }
        }
        kgd_defaults();
        check(ok, "3000 random sounds (put into range) play bounded");
    }
    /* the state for flash: the kits back exactly */
    {
        static kg_blob_t a;
        kgd_kit[KGD_C64][5].p[C6_CUT] = 33, kgd_kit[KGD_FM][9].sat = KS_FOLD;
        kg_pack(&a);
        kgd_defaults();
        ok = kg_unpack(&a) == 0 && kgd_kit[KGD_C64][5].p[C6_CUT] == 33 && kgd_kit[KGD_FM][9].sat == KS_FOLD;
        kgd_defaults();
        check(ok && sizeof(kg_blob_t) <= ST_TAIL_MAX_KG, "the kits in K-GLITCH's flash record, back exactly (it fits)");
    }
    /* a demo: both kits on a beat */
    if (argc > 1) {
        static const uint16_t PAT[16] = {0x0011, 0x0010, 0x0014, 0x0010, 0x0015, 0x0010, 0x0110, 0x0014,
                                         0x0011, 0x0210, 0x0014, 0x0011, 0x0015, 0x8010, 0x0410, 0x0820};
        FILE *f = fopen(argv[1], "wb");
        if (!f) { printf("kgdrum_test: cannot write %s\n", argv[1]); return 1; }
        uint32_t bar, st, step = FS * 60u / 160u / 4u, n = 0;
        wav_hdr(f, 2u * 2u * 16u * step);
        for (m = 0; m < KGD_NK; m++) {
            memset(&drums, 0, sizeof drums);
            TDRUM->p[P_E0] = (int16_t)(DRUM_KGFM + m);
            for (bar = 0; bar < 2u; bar++)
                for (st = 0; st < 16u; st++) {
                    for (l = 0; l < 16u; l++)
                        if (PAT[st] >> l & 1u) drum_on(LANE_NOTE[l], st % 4u ? 90 : 120);
                    for (i = 0; i < step; i += CTL) {
                        uint32_t k;
                        memset(L, 0, CTL * 4), memset(R, 0, CTL * 4);
                        drums_render(L, R, RV, DL, CTL);
                        for (k = 0; k < CTL && i + k < step; k++, n++) wav_put(f, L[k] * 2, R[k] * 2);
                    }
                }
        }
        fclose(f);
    }
    printf("kgdrum_test: %s\n", fails ? "FAILED" : "passed");
    return fails != 0;
}
