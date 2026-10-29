/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include "cp_audio.h"

void cp_audio_convert(const int32_t *in, int16_t *out, uint32_t frames, uint32_t trim_q12)
{
    uint32_t i;
    for (i = 0; i < frames; i++) {
        int64_t mono = ((int64_t)in[2u * i] + in[2u * i + 1u]) >> 1;   /* one speaker: L + R */
        int32_t v = (int32_t)((mono * (int64_t)trim_q12) >> 12) >> 8;   /* 24-bit full scale to 16 */
        if (v > 32767)
            v = 32767;
        else if (v < -32768)
            v = -32768;
        out[2u * i] = out[2u * i + 1u] = (int16_t)v;
    }
}
