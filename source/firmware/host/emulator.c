/* SPDX-License-Identifier: GPL-3.0-only */
/* K-GLITCH browser emulator: the firmware's own UI (ui*.c, panel.c, splash.c) and sound (mix_block: engines, drums,
 * sequencer, punch-in FX, kglitch.c) built for wasm32, with the FM-1's hardware replaced by the page (web/emulator):
 * the screen is a 240 x 240 RGB565 buffer, the buttons, keys and encoders come from the page, the USB-MIDI out goes
 * to a queue the page reads (its MIDI log, and Web MIDI to a real MPC). Flash, user presets and projects are RAM
 * stubs: nothing is kept between sessions. Built by tools/build_emulator.py (clang --target=wasm32, freestanding). */
#include <stdint.h>
#include <string.h>
#define memset felucca_memset
#define memcpy felucca_memcpy
#define memcmp felucca_memcmp
#include "../src/libc.c"
#undef memset
#undef memcpy
#undef memcmp
#include "felucca_tables.h"
static uint8_t user_flash[4][0x14000];
#define SMP_USER_XIP(k) (user_flash[(k) & 3u])
static struct { volatile uint32_t notes, buttons; } fm1_in;
static volatile uint32_t fm1_remote_btn;
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
#define FELUCCA_ARRANGER 1
#include "../src/core.h"
#include "../src/engines.c"
#include "../src/drums.c"
#include "../src/params.c"
#include "../src/voice.c"
#include "../src/slicer.c"
#include "../src/cpu1.h"                        /* no second core in the browser: every unit renders here */
static int cpu1_run(cpu1_job_t fn, uint32_t arg) { (void)fn; (void)arg; return 0; }
static int cpu1_wait(void) { return 0; }
static int cpu1_on(void) { return 0; }
static uint32_t cpu1_ticks(void) { return 0; }
#include "../src/fx.c"
static void fm1_delay_ms(uint32_t ms) { (void)ms; }
#include "../src/usb.c"
#include "../src/midi_uart.c"
#include "../src/arranger.c"
#include "../src/seq.c"

/* ---- the screen, the panel's inputs and lights */
static uint16_t screen[240 * 240];
static uint32_t screen_gen;                     /* bumped on every draw: the page copies the screen when it moved */
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    if (x + w > 240u || y + h > 240u) return;
    screen_gen++;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) screen[(y + j) * 240u + x + i] = p[j * w + i];
}
#include "../src/gfx.c"
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    if (x + w > 240u || y + h > 240u) return;
    screen_gen++;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) screen[(y + j) * 240u + x + i] = swap16(c);
}
static int32_t encs[7];
static uint32_t fm1_ticks(void) { return fm1_ms * 1000u * 24u; }
#define FM1_TICKS_PER_US 24u
static int32_t fm1_enc_take(uint32_t e) { int32_t s = encs[e]; encs[e] = 0; return s; }
static uint8_t fm1_led[16], fm1_led_dim[16], fm1_led_bg[16];
static volatile uint16_t fm1_led_bg_ns;
#define FM1_NCOL 16u
static const int8_t FM1_KEYMAP[5][16];
static void fm1_led_key(uint32_t id, int on) { (void)id; (void)on; }
static uint32_t edges_btn, notes_seen;
static uint32_t fm1_input_edges(int x) { uint32_t e = edges_btn; (void)x; edges_btn = 0; return e; }
static uint32_t fm1_input_note_edges(void) { uint32_t e = fm1_in.notes & ~notes_seen; notes_seen = fm1_in.notes; return e; }
static void fm1_wdt_feed(void) {}
static int32_t fm1_adc_read(int c) { (void)c; return -1; }
static struct { uint32_t magic, stage, page, home, ui_frames; } felucca_dbg;
#define FELUCCA_ICONS 1
#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N], scope_bufr[SCOPE_N];
static uint32_t scope_w;
#include "../src/panel.c"
#include "../src/ui.c"
/* ---- storage: RAM only (a project, user presets: none kept) */
static int project_used(uint32_t i) { (void)i; return 0; }
static void project_save(uint32_t i) { (void)i; ui_message("EMULATOR: NOT SAVED"); }
static void project_load(uint32_t i) { (void)i; ui_message("EMULATOR: NOTHING SAVED"); }
static void project_erase(uint32_t i) { (void)i; }
static void arrangement_save(void) {}
static uint32_t arrangement_ready(void) { return 0; }
static void arrangement_apply(uint32_t s) { (void)s; }
static uint32_t section_bars(uint32_t s) { (void)s; return 1u; }
static void song_backup(void) {}
static void song_restore(void) {}
static void section_store(uint32_t s) { live_sec = (int8_t)s; }
static void section_load(uint32_t s) { live_sec = (int8_t)s; }
static int up_used(uint32_t k) { (void)k; return 0; }
static int up_load(uint32_t k) { (void)k; return 0; }
static int up_load_to(track_t *t, uint32_t k) { (void)t; (void)k; return 0; }
static int up_arp_of(uint32_t k, int16_t *a) { (void)k; (void)a; return 0; }
static uint32_t up_count(void) { return 0; }
static uint32_t up_nth(uint32_t n) { return n; }
static uint32_t up_rank(uint32_t s) { return s; }
static void up_name(uint32_t k, char *b) { (void)k; b[0] = 0; }
static void up_slot_label(char *b, uint32_t k) { fmt_int(b, (int32_t)k + 1); }
static void up_ui(uint32_t op, uint32_t k) { (void)op; (void)k; }
static void settings_save(void) {}
static void kg_store_poll(void) {}                  /* (the page keeps the state: emu_state_get / emu_state_put) */
#include "../src/ui_song.c"
#include "../src/ui_studio.c"
#include "../src/icons.c"
#include "../src/ui_draw.c"
#include "../src/ui_vis.c"
#include "../src/ui_layers.c"
#include "../src/ui_menu.c"
#include "../src/ui_input.c"
#include "../src/splash.c"
static int st_read(uint32_t off, void *dst, uint32_t n) { (void)off, (void)dst, (void)n; return -1; }
static int st_erase(uint32_t off) { (void)off; return -1; }
static int st_prog(uint32_t off, const void *src, uint32_t n) { (void)off, (void)src, (void)n; return -1; }
#include "../src/storage.c"

/* ---- the page's API */
static float emu_out[CTL * 2u * 64u];
static uint32_t emu_midi[256], emu_midi_n;

void emu_init(void)
{
    uint32_t i, k;
    memset(user_flash, 0xFF, sizeof user_flash);
    panel = PANEL_DEFAULT;
    layers_init();
    settings_init();                                    /* (the palette: the greys of the labels, the header, the footer) */
    for (i = 0; i < G_COUNT; i++) song.g[i] = GP[i].def;
    song.g[G_SYNC] = 0;
    song.master_q12 = 4096;
    for (k = 0; k < NTRK; k++) {
        for (i = 0; i < P_E0; i++) trk[k].p[i] = TP[i].def;
        steps_clear(&trk[k]);
    }
    dsu_defaults();
    fm6_init();
    for (i = 0; i < NPART; i++) {
        set_engine_of(&trk[i], TRK_DEF[i][0]);
        apply_preset_to(&trk[i], TRK_DEF[i][1]);
        trk[i].engine = trk[i].eng_req;
    }
    TDRUM->p[P_E0] = DRUM_DEFAULT_KIT;
    kr_generate(KS_JUNGLE);                             /* a break to destroy at once (the ORIGINAL) */
    song.g[G_BPM] = GP[G_BPM].def;                      /* (K-GLITCH's tempo at power-on: 98, not the style's) */
    usb.config = 1;                                     /* (the MIDI out queue runs: the page reads it) */
    sloop_splash();
}
void emu_home(void) { go_home(); ui.force = 1; }
uint16_t *emu_screen(void) { return screen; }
uint32_t emu_screen_gen(void) { return screen_gen; }
float *emu_audio(void) { return emu_out; }
uint32_t emu_rate(void) { return FS; }
uint32_t emu_block(void) { return CTL; }
/* blocks of CTL stereo frames into emu_audio() (float, interleaved); the USB-MIDI out into emu_midi_buf() */
void emu_render(uint32_t blocks)
{
    static int32_t o[CTL * 2u];
    uint32_t b, i;
    if (blocks > 64u) blocks = 64u;
    for (b = 0; b < blocks; b++) {
        mix_block(o, CTL);
        for (i = 0; i < CTL * 2u; i++) emu_out[b * CTL * 2u + i] = (float)o[i] * (1.0f / 32768.0f);
        for (i = 1; i < CTL; i += 2u) {                 /* the scope, as audio.c fills it */
            scope_bufr[scope_w & (SCOPE_N - 1u)] = vis_tap[2u * i + 1u];
            scope_buf[scope_w++ & (SCOPE_N - 1u)] = vis_tap[2u * i];
        }
    }
    while (mo_r != mo_w) {
        if (emu_midi_n < 256u) emu_midi[emu_midi_n++] = midi_out_q[mo_r % MQ];
        mo_r++;
    }
}
uint32_t *emu_midi_buf(void) { return emu_midi; }
uint32_t emu_midi_take(void) { uint32_t n = emu_midi_n; emu_midi_n = 0; return n; }
/* one UI pass: dt ms since the last */
void emu_frame(uint32_t dt)
{
    fm1_ms += dt;
    ui_input();
    ui_leds();
    ui_draw();
}
void emu_button(uint32_t b, uint32_t down)          /* b: B_FX .. B_OCTUP (panel.c) */
{
    uint32_t bit;
    if (b >= NB) return;
    bit = 1u << panel.btn[b];
    if (down) { edges_btn |= bit; fm1_in.buttons |= bit; }
    else fm1_in.buttons &= ~bit;
}
void emu_key(uint32_t k, uint32_t down)             /* k: 0 (F3) .. 26 (G5) */
{
    if (k >= 27u) return;
    if (down) fm1_in.notes |= 1u << k;
    else fm1_in.notes &= ~(1u << k);
}
void emu_enc(uint32_t e, int32_t steps)            /* e: EN_SELECT .. EN_K4 (panel.c) */
{
    if (e < NE) encs[panel.enc[e]] += steps;
}
void emu_master(uint32_t q12) { song.master_q12 = (uint16_t)(q12 > 8192u ? 8192u : q12); }
uint32_t emu_led(uint32_t c) { return c < 16u ? (uint32_t)fm1_led[c] | (uint32_t)fm1_led_dim[c] << 8 : 0u; }
uint32_t emu_playing(void) { return song.playing; }
uint32_t emu_bank(void) { return (uint32_t)song.g[G_KBANK]; }   /* FX > BANK: the white keys' names */
void emu_keys(uint32_t mask) { fm1_in.notes = mask & 0x7FFFFFFu; }   /* the 27 keys held, bit k = key k */
void emu_midi_in(uint32_t st, uint32_t d1, uint32_t d2)   /* a MIDI message from the page (Web MIDI: the MPC's clock) */
{
    if (mi_w - mi_r < MQ) {
        midi_in_q[mi_w % MQ] = (st >> 4 & 15u) | (st & 255u) << 8 | (d1 & 127u) << 16 | (d2 & 127u) << 24;
        mi_w++;
    }
}
/* K-GLITCH's state (kglitch.c kg_blob_t), as the FM-1 keeps it in flash: the page keeps it in the browser */
uint8_t *emu_state_get(void) { kg_pack(&kg_blob); return (uint8_t *)&kg_blob; }
uint32_t emu_state_size(void) { return sizeof kg_blob; }
uint32_t emu_state_sum(void) { kg_blob_t b; kg_pack(&b); return b.sum; }
uint8_t *emu_state_buf(void) { return (uint8_t *)&kg_blob; }
int emu_state_put(void) { return kg_unpack(&kg_blob); }   /* (after the page wrote emu_state_buf) */
