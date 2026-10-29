/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include "cp_audio.h"

#define KNEE 24576                     /* 75 % of 16-bit full scale: linear below */
#define ROOM 8192                      /* above it, x -> KNEE + x * ROOM / (x + ROOM): approaches 32768, never wraps */

void cp_audio_convert(const int32_t *in, int16_t *out, uint32_t frames, uint32_t trim_q12)
{
    uint32_t i;
    for (i = 0; i < frames; i++) {
        int64_t mono = ((int64_t)in[2u * i] + in[2u * i + 1u]) >> 1;   /* one speaker: L + R */
        int64_t g = mono * (int64_t)trim_q12;
        int64_t a = (g < 0 ? -g : g) >> 20;                             /* 24-bit full scale to 16, with the gain */
        if (a > KNEE) {
            int64_t x = a - KNEE;
            a = KNEE + x * ROOM / (x + ROOM);
            if (a > 32767)
                a = 32767;
        }
        out[2u * i] = out[2u * i + 1u] = (int16_t)(g < 0 ? -a : a);   /* the same curve both ways */
    }
}
