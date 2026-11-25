/* SPDX-License-Identifier: MPL-2.0 */
/* CHOPPAH's snare and hi-hat (PRD K-10): two synthesised voices beside the kick (kick.h), each with six settings
 * (0..1), presets, and a lane of the pattern's 16 steps (the app's). LOAF BREAKER: a third, the CLAP: four bursts of
 * band-passed noise (TUNE its band, SNAP how far apart), then a tail (DECAY); TONE brighter. (The RAVE STAB, the
 * HOOVER and the MELODY that followed it: gone in 0.209, kicks and drums only.)
 *   SNARE  a body of two detuned sines dropping onto TUNE, and noise band-passed by TONE; SNAP is how much noise and
 *          crack against the body; DECAY both their tails; DRIVE a soft clip
 *   HAT    six square oscillators at an 808's ratios (TUNE) against noise (TONE), high-passed at CUT; DECAY from a
 *          tight closed hat to an open one; DRIVE
 * Platform-free, like the break engine: settings from the main loop (perc_set, read once a block), hits at sample
 * offsets in the render. */
#pragma once
#include <stdint.h>

enum { PV_SNARE, PV_HAT, PV_CLAP, PV_STAB, PERC_VOICES };   /* (LOAF BREAKER, 0.209: PV_STAB, the MELODY's place,
                                          * empty: kicks and drums only; kept, perc_t's layout in the saves) */
extern const char *const PERC_VOICE_NAME[PERC_VOICES];
typedef struct {
    float level, tune, decay, tone, snap, drive;   /* 0..1; snap is the hat's CUT */
} perc_voice_t;                          /* 24 bytes */
typedef struct {
    perc_voice_t v[PERC_VOICES];
    float volume;                        /* 0..1: the MIXER's PERC fader */
} perc_t;                                /* 100 bytes (the app saves the clap and the stab apart: perc2_t) */

#define PERC_PRESETS 4u
extern const char *const PERC_PRESET_NAME[PERC_VOICES][PERC_PRESETS];
void perc_preset(perc_voice_t *v, uint32_t voice, uint32_t i);
void perc_defaults(perc_t *p);           /* each voice's first preset, volume 0.8 */
float perc_tune_hz(uint32_t voice, float tune);   /* SNARE 120..400 Hz; HAT the lowest of its six, 100..800 Hz;
                                          * CLAP its band, 700..2440 Hz */
float perc_decay_ms(uint32_t voice, float decay); /* to -40 dB: SNARE 60..700 ms, HAT 15..900 ms, CLAP's tail
                                          * 60..900 ms */

void perc_init(void);
void perc_set(const perc_t *p);          /* main loop: read at the next render */
/* LOAF BREAKER (11 Nov 2026, FLEXOR's industrial snares): each drum's extras, 0..100 each (0: as it always was):
 * PX_DROP the body's pitch drop (the snare: x1.7 .. x6), PX_DTIME its time (3 .. 140 ms), PX_DIRT the saturation
 * (DIRT_*), PX_LIFT the tail lifted (an upward compressor), PX_ROOM a short gated room (its level and its length,
 * 40 .. 540 ms); PX_FX, PX_PRE the app's (BEGINNER's FX, the preset), PX_RSV spare */
enum { PX_DROP, PX_DTIME, PX_DIRT, PX_LIFT, PX_ROOM, PX_FX, PX_PRE, PX_RSV, PX_N };
enum { DIRT_SOFT, DIRT_HARD, DIRT_FOLD, DIRT_BITS, DIRT_N };
extern const char *const DIRT_NAME[DIRT_N];
void perc_set_x(uint32_t voice, const uint8_t *x);   /* main loop: read at the next render */
/* LOAF BREAKER (0.9.45, FLEXOR: the site's IMPORT page): a drum's own sample, IMA ADPCM (adpcm.h) of len samples at
 * rate Hz, read where it lies (flash) as it plays; 0: none. It sounds instead of the synthesis while the voice's
 * PX_RSV is PX_SAMPLE: TUNE its pitch (+-12 semitones, 0.5 as recorded), DECAY a fade (all the way up: none), TONE a
 * low-pass (left) or a high-pass (right), then DRIVE, DIRT, LIFT, ROOM and LEVEL as the synthesised */
#define PX_SAMPLE 0xA1u
void perc_set_sample(uint32_t voice, const uint8_t *adpcm, uint32_t len, uint32_t rate);
float perc_sample_ms(float decay);       /* (the sample's fade, to -40 dB: 30 .. 1530 ms; 0: none) */
void perc_hit(uint32_t voice);           /* main loop (a MIDI note): at the next render */
/* render: n samples into out (overwritten); hits at the sample offsets at[0..nat-1] (rising), who[k] the voices
 * (bit PV_*) hit there */
void perc_render(float *out, uint32_t n, const uint16_t *at, const uint8_t *who, uint32_t nat);
void perc_speed(const float *sp);        /* LOAF BREAKER: the VINYL BREAK's, as kick_speed */
/* main loop: one hit of the voice as the screen draws it, offline: px[i] the peak (0..height) of the ith of n columns
 * over its first ms milliseconds, at its level (not the volume) */
void perc_shape(uint32_t voice, const perc_voice_t *v, uint8_t *px, uint32_t n, uint32_t ms, uint32_t height);
/* LOAF BREAKER: one hit drawn as its wave (the drums' big view): px[i] the ith column's sample furthest from 0, signed
 * (-height .. +height), over its first ms milliseconds */
void perc_wave(uint32_t voice, const perc_voice_t *v, int8_t *px, uint32_t n, uint32_t ms, uint32_t height);
void perc_rgb(uint32_t voice, const perc_voice_t *v, uint8_t (*col)[4], uint32_t n, uint32_t ms);   /* (LOAF
                                          * BREAKER: lows, mids, highs, peak a column, as kick_rgb) */
