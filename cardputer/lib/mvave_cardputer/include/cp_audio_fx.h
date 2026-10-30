/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the speaker chain. The Cardputer's speaker is small: below ~180 Hz it plays nothing but
 * rattle, and the FM-1's line level needs gain. So, per sample (mono): the trim; a crossover at hp_hz (the highs
 * go on, the lows are dropped); "virtual bass": the lows' overtones (rectified, kept between hp_hz and ~800 Hz)
 * added back at `bass`, so the ear still hears the bass line; a look-ahead limiter (1.5 ms) to `ceil_db` below
 * full scale; TPDF dither to 16 bits; a soft knee as the last guard. Pure: no Arduino, host-tested. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CP_FX_LOOK 64u                 /* the limiter's look-ahead, samples (1.45 ms at 44.1 kHz) */

typedef struct {
    int on;                            /* 0: cp_audio_convert as before (the A/B) */
    float hp_hz;                       /* the crossover, 60..400 */
    float bass;                        /* the virtual bass, 0..2 (0 off) */
    float ceil_db;                     /* the limiter's ceiling, -12..0 dBFS */
} cp_fx_params_t;

typedef struct {
    float b[3], a[2];                  /* a biquad's coefficients (a0 = 1) */
    float x1, x2, y1, y2;
} cp_biquad_t;

typedef struct {
    cp_fx_params_t p;
    float rate;
    cp_biquad_t hp, lp, hhp, hlp;      /* the crossover; the overtones' band */
    float ceil, att, rel, g;           /* the limiter: ceiling (16-bit units), coefficients, gain */
    float env, ea, er;                 /* the lows' envelope (the virtual bass) */
    float dly[CP_FX_LOOK];             /* the look-ahead delay */
    float qv[CP_FX_LOOK + 1u];         /* the gains wanted over the window: a monotonic queue (min first) */
    uint32_t qi[CP_FX_LOOK + 1u];
    uint32_t qh, qn, n;
    uint32_t rnd;
} cp_fx_t;

void cp_fx_defaults(cp_fx_params_t *p);                       /* on, 180 Hz, bass 0.6, -1 dBFS */
void cp_fx_init(cp_fx_t *f, float rate_hz, const cp_fx_params_t *p);   /* p 0: the defaults */
/* in = interleaved stereo int32 (+-8,388,608), out = interleaved stereo int16 (the same sample twice) */
void cp_fx_process(cp_fx_t *f, const int32_t *in, int16_t *out, uint32_t frames, uint32_t trim_q12);

#ifdef __cplusplus
}
#endif
