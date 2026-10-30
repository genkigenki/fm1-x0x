/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include <math.h>
#include "cp_audio.h"
#include "cp_audio_fx.h"

#define KNEE 24576.0f                  /* the last guard: as cp_audio_convert's soft knee */
#define ROOM 8192.0f
#define VB_TOP 900.0f                  /* the virtual bass's overtones: hp_hz .. here */
#define VB_DRIVE 3.0f                  /* how hard the lows are squared off (their odd overtones) */
#define VB_GAIN 1.5f                   /* bass 1: the overtones about as loud as the lows were */

void cp_fx_defaults(cp_fx_params_t *p)
{
    p->on = 1;
    p->hp_hz = 180.0f;
    p->bass = 0.6f;
    p->ceil_db = -1.0f;
}

static void biquad(cp_biquad_t *q, float rate, float hz, int high)   /* RBJ, Butterworth (Q 0.7071) */
{
    float w = 6.2831853f * hz / rate, c = cosf(w), al = sinf(w) / (2.0f * 0.70710678f), a0 = 1.0f + al;
    float k = high ? (1.0f + c) / 2.0f : (1.0f - c) / 2.0f;
    q->b[0] = k / a0;
    q->b[1] = (high ? -2.0f * k : 2.0f * k) / a0;
    q->b[2] = k / a0;
    q->a[0] = -2.0f * c / a0;
    q->a[1] = (1.0f - al) / a0;
    q->x1 = q->x2 = q->y1 = q->y2 = 0.0f;
}

static inline float bq(cp_biquad_t *q, float x)
{
    float y = q->b[0] * x + q->b[1] * q->x1 + q->b[2] * q->x2 - q->a[0] * q->y1 - q->a[1] * q->y2;
    q->x2 = q->x1, q->x1 = x, q->y2 = q->y1, q->y1 = y;
    return y;
}

void cp_fx_init(cp_fx_t *f, float rate_hz, const cp_fx_params_t *p)
{
    if (p)
        f->p = *p;
    else
        cp_fx_defaults(&f->p);
    f->rate = rate_hz;
    biquad(&f->hp, rate_hz, f->p.hp_hz, 1);
    biquad(&f->lp, rate_hz, f->p.hp_hz, 0);
    biquad(&f->hhp, rate_hz, f->p.hp_hz, 1);
    biquad(&f->hlp, rate_hz, VB_TOP, 0);
    f->ceil = 32768.0f * powf(10.0f, f->p.ceil_db / 20.0f);
    f->att = 1.0f - expf(-5.0f / (float)CP_FX_LOOK);          /* there within the look-ahead */
    f->rel = 1.0f - expf(-1.0f / (0.08f * rate_hz));           /* back up in ~80 ms */
    f->ea = 1.0f - expf(-1.0f / (0.005f * rate_hz));
    f->er = 1.0f - expf(-1.0f / (0.05f * rate_hz));
    f->g = 1.0f;
    f->env = 0.0f;
    for (uint32_t i = 0; i < CP_FX_LOOK; i++)
        f->dly[i] = 0.0f;
    f->qh = f->qn = f->n = 0;
    f->rnd = 0x12345678u;
}

static inline float limit(cp_fx_t *f, float v)                 /* the look-ahead limiter: v in, v of LOOK ago out */
{
    const uint32_t cap = CP_FX_LOOK + 1u;
    float a = fabsf(v), want = a > f->ceil ? f->ceil / a : 1.0f, t, d;
    while (f->qn && f->qv[(f->qh + f->qn - 1u) % cap] >= want)   /* the window's minimum first */
        f->qn--;
    f->qv[(f->qh + f->qn) % cap] = want;
    f->qi[(f->qh + f->qn) % cap] = f->n;
    f->qn++;
    if (f->n - f->qi[f->qh] > CP_FX_LOOK)
        f->qh = (f->qh + 1u) % cap, f->qn--;
    t = f->qv[f->qh];
    f->g += (t - f->g) * (t < f->g ? f->att : f->rel);
    d = f->dly[f->n % CP_FX_LOOK];
    f->dly[f->n % CP_FX_LOOK] = v;
    f->n++;
    return d * f->g;
}

void cp_fx_process(cp_fx_t *f, const int32_t *in, int16_t *out, uint32_t frames, uint32_t trim_q12)
{
    if (!f->p.on) {
        cp_audio_convert(in, out, frames, trim_q12);
        return;
    }
    const float gain = (float)trim_q12 / (4096.0f * 256.0f);    /* 24-bit full scale to 16, with the trim */
    for (uint32_t i = 0; i < frames; i++) {
        float x = ((float)in[2u * i] + (float)in[2u * i + 1u]) * 0.5f * gain;   /* one speaker: L + R */
        float y = bq(&f->hp, x), lo = bq(&f->lp, x);
        if (f->p.bass > 0.0f) {                                 /* the lows' overtones, at the lows' level */
            float a = fabsf(lo), s;
            f->env += (a - f->env) * (a > f->env ? f->ea : f->er);
            s = VB_DRIVE * lo / (f->env + 1.0f);
            s = s > 1.0f ? 1.0f : s < -1.0f ? -1.0f : s;
            y += bq(&f->hlp, bq(&f->hhp, s * f->env)) * f->p.bass * VB_GAIN;
        }
        y = limit(f, y);
        f->rnd = f->rnd * 1664525u + 1013904223u;               /* TPDF dither, +-1 LSB */
        y += ((float)(f->rnd >> 16) + (float)(f->rnd & 0xFFFFu)) * (1.0f / 65536.0f) - 1.0f;
        float a = fabsf(y);
        if (a > KNEE) {
            float e = a - KNEE;
            a = KNEE + e * ROOM / (e + ROOM);
        }
        int32_t v = (int32_t)lrintf(y < 0.0f ? -a : a);
        v = v > 32767 ? 32767 : v < -32767 ? -32767 : v;
        out[2u * i] = out[2u * i + 1u] = (int16_t)v;
    }
}
