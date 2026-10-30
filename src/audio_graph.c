/* SPDX-License-Identifier: GPL-3.0-only */
#include "audio_graph.h"

/* Deliberately not implemented: platform timing and format must be measured first. */
void kg_audio_render(int16_t *stereo_out, uint16_t frames,
                     const kg_controls_t *controls) {
    (void)controls;
    if (!stereo_out) return;
    for (uint32_t i = 0; i < (uint32_t)frames * 2u; ++i) stereo_out[i] = 0;
}
