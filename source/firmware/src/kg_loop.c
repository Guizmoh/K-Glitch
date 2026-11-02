/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH LOOP: a short loop recorded from what plays, then sliced, rearranged, played as grains, destroyed.
 * Included by kglitch.c; runs in the audio interrupt (fx.c mix_block: kl_play before the punch-in effects, so they
 * destroy the loop too; kl_capture after them: what you hear is what is recorded, the loop itself included).
 *
 * Memory: the FM-1 has no room for another buffer. The loop lives in the tempo delay's line (fx.c dly_buf, 128 KB),
 * mono, as 8-bit samples in blocks of 16 that share a scale (block floating point: about 48 dB under each block's
 * peak, and every bit of the 16 for the quiet ones): 126976 samples. At the full 44.1 kHz (the whole spectrum, the
 * hats' air) 2.88 s: 1 bar down to 84 BPM, 2 bars down to 167; the BARS asked that do not fit at 44.1 kHz are
 * recorded at 22.05 kHz (a half-band filter first), 5.76 s: 2 bars down to 84 BPM, 4 down to 167; still too long: fewer
 * bars. Played back with a cubic interpolation. While a loop is held the tempo delay is silent; MODE OFF lets the loop
 * go and gives the delay its line back.
 *
 * LOOP page: REC arms; the recording starts on the next bar (PLAY starts the transport if it is stopped) and lasts
 * BARS (1, 2, 4: as many as fit); then the loop plays, locked to the bar (a new tempo varies its speed). SOURCE MUTE:
 * the tracks are silent while it plays (only the loop, to destroy), MIX: both.
 * MODE LOOP: as recorded. SLICE: cut in 16; each slice of the bar plays a slice of the loop (MAP: in order, or as DICE
 * rearranged them), forwards or backwards, once or retriggered 2, 3, 4 or 8 times (a stutter). GRAIN: four grains at
 * a time from POSITION (with CHAOS some spray), SIZE 10..250 ms, DENSITY (how much they overlap), PITCH -12..+12.
 * The white keys (on LOOP, SLICE, GRAIN): SLICE / LOOP, the slice k from its start at once, on as long as it is held
 * (recording: written into the map); GRAIN, the position k / 16. */

#define KL_N DLY_LEN                              /* the delay line's samples */
enum { KL_OFF, KL_LOOP, KL_SLICE, KL_GRAIN };     /* G_KLMODE */
enum { KLS_EMPTY, KLS_ARMED, KLS_REC, KLS_HAS };
#define KL_NS 16u                                 /* slices */
#define KL_BLK 16u                                /* samples a block (one scale) */
#define KL_NBLK 7936u                             /* blocks: 16 bytes + a 4-bit scale each, in the delay's 131072 bytes */
#define KL_MAXS (KL_NBLK * KL_BLK)                /* 126976 samples */
_Static_assert(KL_MAXS + KL_NBLK / 2u <= sizeof(int16_t) * DLY_LEN, "the loop fits the delay's line");
static uint8_t *const kl_b8 = (uint8_t *)dly_buf;   /* the samples, then the blocks' scales (two a byte) */
static struct {
    volatile uint8_t st;          /* KLS_* (anything but EMPTY: the delay's line is the loop's: fx.c kl_line) */
    uint8_t bars;                 /* the loop's length in bars */
    uint32_t len;                 /* its samples (at FS / rate) */
    uint8_t rate;                 /* 1: 44.1 kHz, 2: 22.05 kHz (longer loops) */
    uint8_t bn;                   /* REC: samples in the block being filled */
    int32_t blk[KL_BLK];          /* .. that block */
    int32_t h[7];                 /* REC at 22.05 kHz: the half-band filter's last inputs */
    uint32_t w;                   /* REC: the next one */
    uint32_t beat0;               /* the transport's beat it started on */
    uint32_t last_beat;
    uint8_t odd;                  /* REC at 22.05 kHz: one input of two */
    uint8_t map[KL_NS], rev[KL_NS], ret[KL_NS];   /* SLICE: which slice plays, backwards, retriggered x ret (1..8) */
    volatile int8_t pad;          /* a white key held: the slice (or the grain position) it plays, -1 none */
    uint8_t pad_new;              /* (it was just pressed: from the slice's start) */
    uint32_t pad_q8;              /* its position in the loop, Q8 */
    struct { uint32_t q8, inc, age, len; uint8_t on; } gr[4];   /* GRAIN */
    uint32_t gr_next;             /* samples to the next grain */
    uint32_t rnd;
    uint8_t ov[110];              /* the waveform's outline (the screen), 0..20 */
    volatile uint8_t ov_new;      /* 2: a new loop, its outline to compute (main loop); 1: to draw again */
    int32_t xl;                   /* the last output (the declick) */
} kl = {.pad = -1, .rnd = 0x2C1B3C6Du,
        .map = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        .ret = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};
static volatile uint8_t kl_padkey;                /* the key held for kl.pad, + 1 (seq.c key_up) */

static uint32_t kl_bar_samples(void) { return beat_samples() * 4u; }
static int kl_line(void) { return kl.st != KLS_EMPTY; }
/* the loop's sample i (the mix's scale) */
static int32_t kl_get(uint32_t i)
{
    uint32_t e = (uint32_t)(kl_b8[KL_MAXS + (i >> 5)] >> ((i >> 2) & 4u)) & 15u;
    return (int32_t)(int8_t)kl_b8[i] << e;
}
/* the loop at position q8 (Q8 samples), cubic (Hermite) */
static int32_t kl_at(uint32_t q8)
{
    uint32_t i = (q8 >> 8) % kl.len, n = kl.len;
    int32_t xm = kl_get(i ? i - 1u : n - 1u), x0 = kl_get(i), x1 = kl_get(i + 1u < n ? i + 1u : 0u),
            x2 = kl_get(i + 2u < n ? i + 2u : i + 2u - n), t = (int32_t)(q8 & 255u);
    int32_t c1 = (x1 - xm) >> 1, c2 = xm - ((5 * x0) >> 1) + 2 * x1 - (x2 >> 1), c3 = ((x2 - xm) >> 1) + ((3 * (x0 - x1)) >> 1);
    c3 = clamp(c3, -(1 << 21), 1 << 21), c2 = clamp(c2, -(1 << 21), 1 << 21);   /* (x 256 three times: 32 bits) */
    return x0 + ((((((c3 * t) >> 8) + c2) * t >> 8) + c1) * t >> 8);
}
/* REC: a block of 16 into the line, with its scale (the smallest that keeps its peak in 8 bits) */
static void kl_put_block(void)
{
    uint32_t b = kl.w / KL_BLK, k, e = 0, pk = 0;
    for (k = 0; k < KL_BLK; k++) {
        uint32_t a = (uint32_t)(kl.blk[k] < 0 ? -kl.blk[k] : kl.blk[k]);
        if (a > pk) pk = a;
    }
    while (e < 15u && (pk + (e ? 1u << (e - 1u) : 0u)) >> e > 127u) e++;
    for (k = 0; k < KL_BLK; k++) {
        int32_t v = e ? (kl.blk[k] + (1 << (e - 1u))) >> e : kl.blk[k];
        kl_b8[kl.w + k] = (uint8_t)(int8_t)clamp(v, -128, 127);
    }
    kl_b8[KL_MAXS + b / 2u] = (uint8_t)((kl_b8[KL_MAXS + b / 2u] & (b & 1u ? 0x0Fu : 0xF0u)) | e << ((b & 1u) * 4u));
    kl.w += KL_BLK;
    kl.bn = 0;
}
static void kl_outline(void)                      /* the waveform's peaks, 110 columns (after a recording) */
{
    uint32_t c, i;
    for (c = 0; c < 110u; c++) {
        uint32_t a = c * kl.len / 110u, b = (c + 1u) * kl.len / 110u, pk = 0;
        for (i = a; i < b; i += 4u) {
            int32_t v = kl_get(i);
            uint32_t m = (uint32_t)(v < 0 ? -v : v);
            if (m > pk) pk = m;
        }
        kl.ov[c] = (uint8_t)(pk * 20u / 32768u);
    }
    kl.ov_new = 1;
}
/* REC (main loop): armed (ui_input.c starts the transport if it is stopped) */
static void kl_arm(void)
{
    kl.st = KLS_ARMED;
    kl.last_beat = 0xFFFFFFFFu;
}
/* MODE OFF (main loop): the loop let go, the delay's line back (silent, from zero) */
static void kl_free(void)
{
    fm1_irq_off();
    kl.st = KLS_EMPTY;
    memset(dly_buf, 0, sizeof dly_buf);
    fm1_irq_on();
}
/* DICE (main loop): the slices rearranged, some backwards, some retriggered; CHAOS says how much */
static void kl_dice(void)
{
    static const uint8_t RET[5] = {1, 2, 3, 4, 8};
    uint32_t i, C = kg_pct(G_KCHAOS);
    kl.rnd ^= fm1_ms * 2654435761u;
    for (i = 0; i < KL_NS; i++) {
        uint32_t r = kg_rand(&kl.rnd);
        kl.map[i] = (uint8_t)(r % 100u < 20u + C * 6u / 10u ? (r >> 8) % KL_NS : i);
        kl.rev[i] = (uint8_t)((r >> 16) % 100u < C / 3u);
        kl.ret[i] = (r >> 20) % 100u < C / 2u ? RET[1u + (r >> 24) % 4u] : 1u;
    }
}
static void kl_reset(void)                        /* the slices in order, forwards, once */
{
    uint32_t i;
    for (i = 0; i < KL_NS; i++) kl.map[i] = (uint8_t)i, kl.rev[i] = 0, kl.ret[i] = 1;
}
/* a white key on the LOOP pages (seq.c key_down / key_up) */
static void kl_pad_down(uint32_t w, uint32_t k)
{
    kl.pad = (int8_t)(w & 15u);
    kl.pad_new = 1;
    kl_padkey = (uint8_t)(k + 1u);
}
static void kl_pad_up(void)
{
    kl.pad = -1;
    kl_padkey = 0;
}

/* the loop into the mix (before the punch-in effects) */
static void kl_play(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t mode = (uint32_t)song.g[G_KLMODE] & 3u, i, total, rq8, pos, L8, len8;
    if (kl.st != KLS_HAS || mode == KL_OFF || !song.playing || !kl.len)
        return;
    total = kl_bar_samples() * kl.bars;
    rq8 = (kl.len << 8) / (total ? total : 1u);   /* Q8 loop samples a sample (once a block; len < 2^17: 32 bits) */
    pos = ((clk_beat - kl.beat0) % (4u * kl.bars)) * beat_samples() + clk_samples();
    len8 = kl.len << 8;
    L8 = len8 / KL_NS;
    if (song.g[G_KLSRC] == 0)                     /* SOURCE MUTE: only the loop */
        for (i = 0; i < n; i++) l[i] = r[i] = 0;
    if (kl.pad >= 0 && kl.pad_new) {              /* a key: its slice from the start */
        kl.pad_new = 0;
        kl.pad_q8 = (uint32_t)kl.pad * L8;
    }
    for (i = 0; i < n; i++, pos++) {
        uint32_t q8 = pos * rq8 % len8, s = q8 / L8, f = q8 - s * L8, src;
        int32_t y, env = 256;
        if (mode == KL_GRAIN) {
            uint32_t g, sz = (10u + kg_pct(G_KGSIZE) * 240u / 100u) * (FS / 1000u), live = 0;
            if (!kl.gr_next--) {                   /* a new grain */
                uint32_t dens = kg_pct(G_KGDENS), at = kl.pad >= 0 ? (uint32_t)kl.pad * (kl.len / 16u)
                                                                    : kg_pct(G_KGPOS) * (kl.len - 1u) / 100u;
                uint32_t spray = kg_pct(G_KCHAOS) * (kl.len / 8u) / 100u, rr = kg_rand(&kl.rnd);
                kl.gr_next = sz / (1u + dens / 14u);
                if (spray) at = (at + rr % spray) % kl.len;
                for (g = 0; g < 4u; g++)
                    if (!kl.gr[g].on) {
                        kl.gr[g].on = 1;
                        kl.gr[g].q8 = at << 8;
                        kl.gr[g].inc = (rq8 * (uint32_t)dd_semi(song.g[G_KGPIT])) >> 16;   /* (2^9 x 2^18: 32 bits) */
                        kl.gr[g].age = 0;
                        kl.gr[g].len = sz;
                        break;
                    }
            }
            y = 0;
            for (g = 0; g < 4u; g++)
                if (kl.gr[g].on) {
                    uint32_t a = kl.gr[g].age, h = kl.gr[g].len / 2u;
                    int32_t w = (int32_t)((a < h ? a : kl.gr[g].len - a) * 256u / (h ? h : 1u));
                    y += (kl_at(kl.gr[g].q8) * w) >> 8;
                    kl.gr[g].q8 = (kl.gr[g].q8 + kl.gr[g].inc) % len8;
                    if (++kl.gr[g].age >= kl.gr[g].len) kl.gr[g].on = 0;
                    live++;
                }
            y = live > 2u ? y * 2 / (int32_t)live : y;
        } else if (kl.pad >= 0) {                  /* a key held: its slice, then on */
            y = kl_at(kl.pad_q8);
            f = kl.pad_q8 % L8;
            kl.pad_q8 = (kl.pad_q8 + rq8) % len8;
            if (song.rec) kl.map[s] = (uint8_t)(kl.pad_q8 / L8);   /* (REC: written into the map) */
        } else if (mode == KL_SLICE) {
            uint32_t rt = kl.ret[s] ? kl.ret[s] : 1u, ff = f * rt % L8;
            src = kl.map[s] % KL_NS;
            if (kl.rev[s]) ff = L8 - 1u - ff;
            y = kl_at(src * L8 + ff);
            f = f * rt % L8;
        } else
            y = kl_at(q8);
        if (mode != KL_GRAIN) {                    /* the seams: 64 samples in and out */
            uint32_t e = 64u << 8;
            if (f < e) env = (int32_t)(f >> 6);
            else if (L8 - f < e) env = (int32_t)((L8 - f) >> 6);
            if (env > 256) env = 256;
            y = (y * env) >> 8;
        }
        l[i] += y;                                /* (stored at the mix's level) */
        r[i] += y;
    }
}

/* the mix into the loop while it records (after the effects): two samples make one */
static void kl_capture(const int32_t *l, const int32_t *r, uint32_t n)
{
    uint32_t i;
    if (kl.st == KLS_ARMED) {
        uint32_t b = clk_beat;
        if (!song.playing) return;
        if (b % 4u != 0u || b == kl.last_beat || clk_pos > (uint32_t)FS * 60u / 8u) {   /* (the downbeat's first 32nd) */
            kl.last_beat = b;
            return;
        }
        {   /* BARS: at 44.1 kHz if they fit, else at 22.05; else as many as fit */
            uint32_t want = 1u << ((uint32_t)song.g[G_KLBARS] % 3u), bar = kl_bar_samples();
            kl.rate = (uint8_t)(want * bar <= KL_MAXS ? 1u : 2u);
            bar /= kl.rate;
            while (want > 1u && want * bar > KL_MAXS) want >>= 1;
            kl.bars = (uint8_t)want;
            kl.len = (want * bar > KL_MAXS ? KL_MAXS : want * bar) / KL_BLK * KL_BLK;
        }
        kl.beat0 = b;
        kl.w = 0;
        kl.bn = 0;
        kl.odd = 0;
        memset(kl.h, 0, sizeof kl.h);
        kl.st = KLS_REC;
    }
    if (kl.st != KLS_REC) return;
    if (!song.playing) {                           /* stopped while recording: nothing kept */
        kl.st = KLS_EMPTY;
        return;
    }
    for (i = 0; i < n && kl.w < kl.len; i++) {
        int32_t m = clamp((l[i] + r[i]) >> 1, -(1 << 21), 1 << 21);
        if (kl.rate == 2u) {                       /* 22.05 kHz: a half-band low-pass (-1 0 9 16 9 0 -1) / 32, one of two */
            kl.h[0] = kl.h[1], kl.h[1] = kl.h[2], kl.h[2] = kl.h[3], kl.h[3] = kl.h[4], kl.h[4] = kl.h[5], kl.h[5] = kl.h[6];
            kl.h[6] = m;
            kl.odd ^= 1u;
            if (kl.odd) continue;
            m = (16 * kl.h[3] + 9 * (kl.h[2] + kl.h[4]) - (kl.h[0] + kl.h[6])) >> 5;
        }
        kl.blk[kl.bn++] = m;
        if (kl.bn == KL_BLK) kl_put_block();
    }
    if (kl.w >= kl.len) {
        kl.st = KLS_HAS;
        kl.ov_new = 2;                             /* (the screen draws the outline: kl_outline, not here) */
        if (song.g[G_KLMODE] == KL_OFF)
            song.g[G_KLMODE] = KL_LOOP;
    }
}
