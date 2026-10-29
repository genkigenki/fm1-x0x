/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include "cp_scale.h"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2")     /* the blit runs for every pixel of every frame; Arduino builds with -Os */
#endif

void cp_ymap_build(cp_ymap_t *map, uint32_t src_h, uint32_t dst_h)
{
    uint32_t t;
    for (t = 0; t < dst_h; t++) {
        /* source position s = (t + 0.5) * src_h / dst_h - 0.5, in units of 1 / (2 dst_h) */
        int32_t num = (int32_t)((2u * t + 1u) * src_h) - (int32_t)dst_h;
        uint32_t r0, frac, w1;
        if (num < 0)
            num = 0;
        r0 = (uint32_t)num / (2u * dst_h);
        frac = (uint32_t)num % (2u * dst_h);
        w1 = (frac * 32u + dst_h) / (2u * dst_h);
        if (w1 > 31u)
            w1 = 31u;
        if (r0 >= src_h)
            r0 = src_h - 1u;
        map[t].r0 = (uint8_t)r0;
        map[t].r1 = (uint8_t)(w1 && r0 + 1u < src_h ? r0 + 1u : r0);
        map[t].w1 = (uint8_t)(map[t].r1 == r0 ? 0u : w1);
    }
}

void cp_ymap_crop(cp_ymap_t *map, uint32_t dst_h, uint32_t dy)
{
    uint32_t t;
    for (t = 0; t < dst_h; t++) {
        uint32_t r = dy + t < CP_SRC_EXTENT ? dy + t : CP_SRC_EXTENT - 1u;
        map[t].r0 = map[t].r1 = (uint8_t)r;
        map[t].w1 = 0;
    }
}

void cp_xmap_build(uint8_t *xmap, uint32_t src_w, uint32_t dst_w)
{
    uint32_t i;
    for (i = 0; i < dst_w; i++) {
        uint32_t s = dst_w > 1u ? (i * (src_w - 1u) + (dst_w - 1u) / 2u) / (dst_w - 1u) : 0u;
        xmap[i] = (uint8_t)(s < src_w ? s : src_w - 1u);
    }
}

uint16_t cp_blend565(uint16_t a, uint16_t b, uint32_t w1)
{
    uint32_t ea = (a & 0xF81Fu) | ((uint32_t)(a & 0x07E0u) << 16);
    uint32_t eb = (b & 0xF81Fu) | ((uint32_t)(b & 0x07E0u) << 16);
    uint32_t m = ((ea * (32u - w1) + eb * w1) >> 5) & 0x07E0F81Fu;
    return (uint16_t)((m & 0xF81Fu) | (m >> 16));
}

static inline uint16_t src_px(const uint16_t *row, uint32_t i, uint16_t c, int swap)
{
    uint16_t v;
    if (!row)
        return c;
    v = row[i];
    return swap ? (uint16_t)((v >> 8) | (v << 8)) : v;
}

uint32_t cp_scale_row(uint16_t *o, const cp_ymap_t *m, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      const uint16_t *px, uint16_t c, int swap, const uint8_t *cols, uint32_t ncols)
{
    uint32_t stride = w, cw, ch, r0 = m->r0, r1 = m->r1, w1 = m->w1;
    int in0, in1;
    const uint16_t *a, *b;
    if (x >= CP_SRC_EXTENT || y >= CP_SRC_EXTENT || !w || !h)
        return 0;
    cw = x + w > CP_SRC_EXTENT ? CP_SRC_EXTENT - x : w;
    ch = y + h > CP_SRC_EXTENT ? CP_SRC_EXTENT - y : h;
    in0 = r0 >= y && r0 < y + ch;
    in1 = r1 >= y && r1 < y + ch;
    if (!in0 && !in1)
        return 0;
    if (!in0 || !in1) {                   /* one source row inside this rectangle: that row (nearest) */
        r0 = r1 = in0 ? r0 : r1;
        w1 = 0;
    }
    a = px ? px + (r0 - y) * stride : 0;
    b = px ? px + (r1 - y) * stride : 0;
    if (cols) {
        uint32_t i;
        for (i = 0; i < ncols; i++) {
            uint32_t s = cols[i];
            if (s < x || s >= x + cw)
                continue;
            o[i] = w1 ? cp_blend565(src_px(a, s - x, c, swap), src_px(b, s - x, c, swap), w1) : src_px(a, s - x, c, swap);
        }
    } else {
        {
            uint32_t i, n = ncols < cw ? ncols : cw;
            if (!px) {                                        /* fill: the colour blends with itself */
                for (i = 0; i < n; i++)
                    o[i] = c;
            } else if (!w1) {                                 /* one source row */
                if (swap)
                    for (i = 0; i < n; i++)
                        o[i] = (uint16_t)((a[i] >> 8) | (a[i] << 8));
                else
                    for (i = 0; i < n; i++)
                        o[i] = a[i];
            } else {                                          /* two rows, blended in one 32-bit multiply each */
                uint32_t wa = 32u - w1;
                for (i = 0; i < n; i++) {
                    uint32_t va = a[i], vb = b[i], ea, eb, mm;
                    if (swap) {
                        va = ((va >> 8) | (va << 8)) & 0xFFFFu;
                        vb = ((vb >> 8) | (vb << 8)) & 0xFFFFu;
                    }
                    ea = (va & 0xF81Fu) | ((va & 0x07E0u) << 16);
                    eb = (vb & 0xF81Fu) | ((vb & 0x07E0u) << 16);
                    mm = ((ea * wa + eb * w1) >> 5) & 0x07E0F81Fu;
                    o[i] = (uint16_t)((mm & 0xF81Fu) | (mm >> 16));
                }
            }
        }
    }
    return 1;
}

static uint32_t blit(uint16_t *fb, uint32_t fb_w, uint32_t dst_h, const cp_ymap_t *map, const uint8_t *xmap,
                     uint32_t dst_w, uint32_t x_off, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     const uint16_t *px, uint16_t c, int swap, uint32_t *y0, uint32_t *y1)
{
    uint32_t t, rows = 0;
    for (t = 0; t < dst_h; t++) {
        uint32_t ok;
        if (xmap)
            ok = cp_scale_row(fb + t * fb_w + x_off, &map[t], x, y, w, h, px, c, swap, xmap,
                              x_off < fb_w ? (dst_w < fb_w - x_off ? dst_w : fb_w - x_off) : 0u);
        else
            ok = x < fb_w ? cp_scale_row(fb + t * fb_w + x, &map[t], x, y, w, h, px, c, swap, 0, fb_w - x) : 0u;
        if (!ok)
            continue;
        if (!rows || t < *y0)
            *y0 = t;
        if (!rows || t > *y1)
            *y1 = t;
        rows++;
    }
    return rows;
}

uint32_t cp_scale_blit(uint16_t *fb, uint32_t fb_w, uint32_t dst_h, const cp_ymap_t *map, uint32_t x, uint32_t y,
                       uint32_t w, uint32_t h, const uint16_t *px, uint16_t c, int swap, uint32_t *y0, uint32_t *y1)
{
    return blit(fb, fb_w, dst_h, map, 0, 0, 0, x, y, w, h, px, c, swap, y0, y1);
}

uint32_t cp_scale_blit_xy(uint16_t *fb, uint32_t fb_w, uint32_t dst_h, const cp_ymap_t *map, const uint8_t *xmap,
                          uint32_t dst_w, uint32_t x_off, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                          const uint16_t *px, uint16_t c, int swap, uint32_t *y0, uint32_t *y1)
{
    return blit(fb, fb_w, dst_h, map, xmap, dst_w, x_off, x, y, w, h, px, c, swap, y0, y1);
}
