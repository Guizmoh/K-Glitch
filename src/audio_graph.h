/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef KGLITCH_AUDIO_GRAPH_H
#define KGLITCH_AUDIO_GRAPH_H

#include <stdint.h>

/* Clean-room interface sketch; no renderer or hardware binding exists yet. */
typedef struct {
    uint32_t sample_rate_hz;
    uint16_t frames_per_block;
    uint16_t channels;
} kg_audio_format_t;

typedef struct {
    int16_t *samples;
    uint32_t capacity_frames;
    volatile uint32_t write_frame;
} kg_capture_ring_t;

typedef struct {
    uint16_t loop_frames;
    uint16_t stutter_division;
    uint8_t reverse;
    uint8_t freeze;
    uint8_t crush_bits;
    uint8_t sample_rate_div;
    uint8_t probability;
    uint8_t intensity;
} kg_controls_t;

/* Future callback contract: bounded work only; no allocation or I/O. */
void kg_audio_render(int16_t *stereo_out, uint16_t frames,
                     const kg_controls_t *controls);

#endif
