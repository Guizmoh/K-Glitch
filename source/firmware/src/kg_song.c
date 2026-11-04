/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH: each section (SAVE > PROJECT A..D, the song's files) keeps its own effects: the effect lane / automation
 * (both layers, intensities, probabilities, filter notes), each effect's INTENSITY, the slices' map and K-GLITCH's
 * settings (bank, CHAOS, filter, DJ filter...). SAVE writes them with the section, LOAD brings them back (none saved:
 * cleared), NEW clears them; a recorded loop never goes from one song to the next (RAM only: let go).
 * Where: the flash sector 0xFB000 (free in SLOOP), four records of 960 B, none in its last 256 B (where an update
 * record could be looked for, firmware/loader). Not in the sections' own sectors: a project fills them.
 * The drum kits, OUT (FM-1 / MPC) and the MPC channel stay the FM-1's (settings). Main loop only. */
#define KGS_BASE 0xFB000u
#define KGS_REC 960u
#define KGS_MAGIC 0x31534B47u                     /* "GKS1" */
#define KGS_NG (G_COUNT - G_KCHAOS)
typedef struct {
    uint32_t magic;
    uint16_t ng, slot;
    int8_t seq[KG_STEPS], seq2[KG_STEPS];
    uint8_t sint[KG_STEPS], prob[KG_STEPS], fnote[KG_STEPS], fmode[KG_STEPS];
    uint8_t dint[DD_N], map[KL_NS], rev[KL_NS], ret[KL_NS];
    int16_t g[KGS_NG];
    uint32_t sum;
} kgs_t;
_Static_assert(sizeof(kgs_t) <= KGS_REC && 4u * KGS_REC <= 4096u - 256u, "four records in the sector, its end free");
static kgs_t kgs_buf;

static int kgs_kept(uint32_t id)                  /* a global the section keeps (the others: the FM-1's, or live) */
{
    return id != G_KTGT && id != G_KCCCH && id != G_KLMODE && id != G_KLREC && !(id >= G_KDKIT && id <= G_KXREV);
}
static uint32_t kgs_sum(const kgs_t *b)
{
    const uint8_t *p = (const uint8_t *)b;
    uint32_t h = 2166136261u, i;
    for (i = 0; i < sizeof *b - 4u; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}
/* a new song: no effect playing, the lane empty, every INTENSITY and K-GLITCH setting at its default, the slices in
 * order, no loop */
static void kgs_clear(void)
{
    uint32_t i;
    kg_clean();
    kg_clear();
    for (i = 0; i < DD_N; i++) kd_int[i] = 50;
    kl_reset();
    for (i = G_KCHAOS; i < G_COUNT; i++)
        if (kgs_kept(i))
            song.g[i] = GP[i].def;
    song.g[G_KLMODE] = 0;
    if (kl.st != KLS_EMPTY)
        kl_free();
    kgd_ui_sync();
}
static void kgs_pack(kgs_t *b, uint32_t slot)
{
    uint32_t i;
    memset(b, 0, sizeof *b);
    b->magic = KGS_MAGIC;
    b->ng = KGS_NG;
    b->slot = (uint16_t)slot;
    for (i = 0; i < KG_STEPS; i++)
        b->seq[i] = kg.seq[i], b->seq2[i] = kg.seq2[i], b->sint[i] = kg.sint[i], b->prob[i] = kg.prob[i],
        b->fnote[i] = kg.fnote[i], b->fmode[i] = kg.fmode[i];
    for (i = 0; i < DD_N; i++) b->dint[i] = kd_int[i];
    for (i = 0; i < KL_NS; i++) b->map[i] = kl.map[i], b->rev[i] = kl.rev[i], b->ret[i] = kl.ret[i];
    for (i = 0; i < KGS_NG; i++) b->g[i] = song.g[G_KCHAOS + i];
    b->sum = kgs_sum(b);
}
static int kgs_take(const kgs_t *b, uint32_t slot)   /* 0: taken; -1: not one (nothing changed) */
{
    uint32_t i, ng;
    if (b->magic != KGS_MAGIC || b->slot != slot || b->sum != kgs_sum(b))
        return -1;
    kgs_clear();
    for (i = 0; i < KG_STEPS; i++) {
        kg.seq[i] = b->seq[i] < (int8_t)KG_NFX ? b->seq[i] : -1;
        kg.seq2[i] = b->seq2[i] < (int8_t)KG_NFX ? b->seq2[i] : -1;
        kg.sint[i] = b->sint[i] <= 101u ? b->sint[i] : 0u;
        kg.prob[i] = b->prob[i] <= 100u ? b->prob[i] : 100u;
        kg.fnote[i] = b->fnote[i] >= 24u && b->fnote[i] <= 120u ? b->fnote[i] : 0u;
        kg.fmode[i] = kg.fnote[i] && (b->fmode[i] & 15u) <= KF_N ? b->fmode[i] : 0u;
    }
    for (i = 0; i < DD_N; i++) kd_int[i] = b->dint[i] <= 100u ? b->dint[i] : 50u;
    for (i = 0; i < KL_NS; i++) {
        kl.map[i] = (uint8_t)(b->map[i] % KL_NS);
        kl.rev[i] = (uint8_t)(b->rev[i] != 0);
        kl.ret[i] = b->ret[i] >= 1u && b->ret[i] <= 8u ? b->ret[i] : 1u;
    }
    ng = b->ng < KGS_NG ? b->ng : KGS_NG;
    for (i = 0; i < ng; i++)
        if (kgs_kept(G_KCHAOS + i))
            song.g[G_KCHAOS + i] = (int16_t)clamp(b->g[i], GP[G_KCHAOS + i].min, GP[G_KCHAOS + i].max);
    kgd_ui_sync();
    return 0;
}
static int kgs_write(uint32_t slot, const kgs_t *b)   /* b 0: the slot's record erased; 0 = ok */
{
#if FELUCCA_FLASH
    static uint8_t keep[3u * KGS_REC];             /* the other three records, rewritten with the sector */
    uint32_t i, k = 0;
    int rc;
    if (!flash_ok)
        return -1;
    for (i = 0; i < 4u; i++)
        if (i != slot && st_read(KGS_BASE + i * KGS_REC, keep + k++ * KGS_REC, KGS_REC))
            return -1;
    if ((rc = st_erase(KGS_BASE)) != 0)
        return rc;
    for (i = 0, k = 0; i < 4u; i++) {
        const uint8_t *src = i == slot ? (const uint8_t *)b : keep + k++ * KGS_REC;
        if (!src || ((const uint32_t *)src)[0] == 0xFFFFFFFFu)
            continue;                              /* (erased: stays erased) */
        if ((rc = st_prog(KGS_BASE + i * KGS_REC, src, i == slot ? (uint32_t)sizeof(kgs_t) : KGS_REC)) != 0)
            return rc;
    }
    return 0;
#else
    (void)slot, (void)b;
    return -1;
#endif
}
static void kgs_save(uint32_t slot)               /* SAVE: the section's effects with it */
{
    kgs_pack(&kgs_buf, slot & 3u);
    if (kgs_write(slot & 3u, &kgs_buf))
        ui_message("FX SAVE ERROR");
}
static void kgs_load(uint32_t slot)               /* LOAD: the section's effects (none saved: cleared) */
{
#if FELUCCA_FLASH
    if (flash_ok && st_read(KGS_BASE + (slot & 3u) * KGS_REC, &kgs_buf, sizeof kgs_buf) == 0 &&
        kgs_take(&kgs_buf, slot & 3u) == 0)
        return;
#endif
    kgs_clear();
}
static void kgs_erase(uint32_t slot) { kgs_write(slot & 3u, 0); }
