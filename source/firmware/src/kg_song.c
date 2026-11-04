/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH: each section (SAVE > PROJECT A..D, the song's files) keeps its own effects: the effect lane / automation
 * (both layers, intensities, probabilities, filter notes), each effect's INTENSITY, the slices' map and K-GLITCH's
 * settings (bank, CHAOS, filter, DJ filter...). SAVE writes them with the section, LOAD brings them back (none saved:
 * cleared), NEW clears them; a recorded loop never goes from one song to the next (RAM only: let go).
 * Where: four records of 960 B a sector, none in its last 256 B (where an update record could be looked for,
 * firmware/loader): songs A..D in 0xFB000 (free in SLOOP), E..P in 0xEB000..0xEDFFF (USR4's flash, after songs O, P).
 * Not in the songs' own sectors: a project fills them. 0.9.4: the song's NAME (12 characters) at the record's end,
 * with its own check (a 0.9.2 record: no name).
 * The drum kits stay the FM-1's (settings). Main loop only. */
#define KGS_REC 960u
static uint32_t kgs_sec(uint32_t slot) { slot %= NSONG; return slot < 4u ? 0xFB000u : 0xEB000u + (slot / 4u - 1u) * 0x1000u; }
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
_Static_assert(3u * KGS_REC <= ST_PAYLOAD_MAX && sizeof(kgs_t) <= KGS_REC && 4u * KGS_REC <= 4096u - 256u, "four records in the sector, its end free");
typedef struct {                                    /* the record's end: the song's name */
    char name[KGS_NAME];
    uint32_t sum;
} kgs_name_t;
#define KGS_NAME_OFF (KGS_REC - sizeof(kgs_name_t))
_Static_assert(sizeof(kgs_t) <= KGS_NAME_OFF, "the name after the effects");
static kgs_t kgs_buf;
static char song_name[KGS_NAME + 1];                /* the working song's name (SAVE > NAME), saved with it */
static char kgs_names[NSONG][KGS_NAME + 1];         /* the 16 songs' names, as the flash has them (song_names_read) */
static uint8_t kgs_names_ok;
static uint32_t kgs_name_sum(const char *n)
{
    uint32_t h = 0x4E414D45u, i;
    for (i = 0; i < KGS_NAME; i++) h = (h ^ (uint8_t)n[i]) * 16777619u;
    return h;
}
UI_SMALL static void kgs_name_get(uint32_t slot, char *out)   /* the name in flash ("" none) */
{
    kgs_name_t nm;
    uint32_t i;
    out[0] = 0;
#if FELUCCA_FLASH
    if (!flash_ok || st_read(kgs_sec(slot) + (slot % 4u) * KGS_REC + KGS_NAME_OFF, &nm, sizeof nm) ||
        nm.sum != kgs_name_sum(nm.name))
        return;
    for (i = 0; i < KGS_NAME; i++) out[i] = nm.name[i] >= 32 && nm.name[i] < 127 ? nm.name[i] : ' ';
    out[KGS_NAME] = 0;
    for (i = KGS_NAME; i && out[i - 1u] == ' '; i--) out[i - 1u] = 0;   /* (no trailing spaces) */
#else
    (void)slot, (void)nm, (void)i;
#endif
}
UI_SMALL static const char *song_name_of(uint32_t slot)
{
    uint32_t i;
    if (!kgs_names_ok) {
        for (i = 0; i < NSONG; i++) kgs_name_get(i, kgs_names[i]);
        kgs_names_ok = 1;
    }
    return kgs_names[slot % NSONG];
}

UI_SMALL static int kgs_kept(uint32_t id)                  /* a global the section keeps (the others: the FM-1's, or live) */
{
    return id != G_KTGT && id != G_KCCCH && id != G_KLMODE && id != G_KLREC && !(id >= G_KDKIT && id <= G_KXREV);
}
UI_SMALL static uint32_t kgs_sum(const kgs_t *b)
{
    const uint8_t *p = (const uint8_t *)b;
    uint32_t h = 2166136261u, i;
    for (i = 0; i < sizeof *b - 4u; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}
/* a new song: no effect playing, the lane empty, every INTENSITY and K-GLITCH setting at its default, the slices in
 * order, no loop */
UI_SMALL static void kgs_clear(void)
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
    song_name[0] = 0;
    kgu_sync();                                    /* (the undo history: a new song is not a change of the lane) */
}
UI_SMALL static void kgs_pack(kgs_t *b, uint32_t slot)
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
UI_SMALL static int kgs_take(const kgs_t *b, uint32_t slot)   /* 0: taken; -1: not one (nothing changed) */
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
    kgu_sync();
    return 0;
}
UI_SMALL static int kgs_write(uint32_t slot, const kgs_t *b)   /* b 0: the slot's record erased; 0 = ok */
{
#if FELUCCA_FLASH
    uint8_t *keep = (uint8_t *)st_buf;             /* the other three records, rewritten with the sector (the
                                                    * store's scratch: free, the song's own save is done) */
    uint32_t i, k = 0, base = kgs_sec(slot), at = slot % 4u;
    int rc;
    if (!flash_ok)
        return -1;
    for (i = 0; i < 4u; i++)
        if (i != at && st_read(base + i * KGS_REC, keep + k++ * KGS_REC, KGS_REC))
            return -1;
    if ((rc = st_erase(base)) != 0)
        return rc;
    for (i = 0, k = 0; i < 4u; i++) {
        const uint8_t *src = i == at ? (const uint8_t *)b : keep + k++ * KGS_REC;
        if (!src || ((const uint32_t *)src)[0] == 0xFFFFFFFFu)
            continue;                              /* (erased: stays erased) */
        if (i == at) {
            kgs_name_t nm;
            uint32_t j;
            for (j = 0; j < KGS_NAME; j++) nm.name[j] = j < str_len(song_name) ? song_name[j] : ' ';
            nm.sum = kgs_name_sum(nm.name);
            if ((rc = st_prog(base + i * KGS_REC, src, (uint32_t)sizeof(kgs_t))) != 0 ||
                (rc = st_prog(base + i * KGS_REC + KGS_NAME_OFF, &nm, sizeof nm)) != 0)
                return rc;
        } else if ((rc = st_prog(base + i * KGS_REC, src, KGS_REC)) != 0)
            return rc;
    }
    kgs_names_ok = 0;                              /* (read again) */
    return 0;
#else
    (void)slot, (void)b;
    return -1;
#endif
}
UI_SMALL static void kgs_save(uint32_t slot)               /* SAVE: the song's effects and name with it */
{
    kgs_pack(&kgs_buf, slot % NSONG);
    if (kgs_write(slot % NSONG, &kgs_buf))
        ui_message("FX SAVE ERROR");
}
UI_SMALL static void kgs_load(uint32_t slot)               /* LOAD: the song's effects (none saved: cleared), its name */
{
    char nm[KGS_NAME + 1];
    slot %= NSONG;
    kgs_name_get(slot, nm);
#if FELUCCA_FLASH
    if (flash_ok && st_read(kgs_sec(slot) + (slot % 4u) * KGS_REC, &kgs_buf, sizeof kgs_buf) == 0 &&
        kgs_take(&kgs_buf, slot) == 0) {
        str_cpy(song_name, nm, sizeof song_name);
        return;
    }
#endif
    kgs_clear();
    str_cpy(song_name, nm, sizeof song_name);
}
UI_SMALL static void kgs_erase(uint32_t slot) { kgs_write(slot % NSONG, 0); }
