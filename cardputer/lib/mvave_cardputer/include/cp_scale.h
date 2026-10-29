/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the screen scaler, pure C (no Arduino). The firmware draws a 240 x 240 screen; the
 * Cardputer shows 240 x 135. A row map squeezes the source rows into fewer target rows, each target row a
 * 2-tap blend of two source rows (weights in 1/32); an optional column map shrinks the width (nearest).
 * Colours are RGB565 in the CPU's order; `swap` byte-swaps blitted pixels that come in the panel's order. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t r0, r1, w1;   /* source rows, weight of r1 in 1/32 (0..31); w1 == 0: r1 == r0 */
} cp_ymap_t;

void cp_ymap_build(cp_ymap_t *map, uint32_t src_h, uint32_t dst_h);   /* src_h <= 256 */
void cp_ymap_crop(cp_ymap_t *map, uint32_t dst_h, uint32_t dy);       /* row t shows source row dy + t */
void cp_xmap_build(uint8_t *xmap, uint32_t src_w, uint32_t dst_w);     /* nearest, centres */
uint16_t cp_blend565(uint16_t a, uint16_t b, uint32_t w1);

/* Draw a source rectangle (x, y, w, h in source coordinates; px row stride w, or px NULL: fill with c) into the
 * target framebuffer fb (fb_w pixels a row, dst_h target rows). Clamped to the 240 x 240 source and the target.
 * Returns the target rows written; *y0..*y1 the dirty range (untouched when 0 rows). */
uint32_t cp_scale_blit(uint16_t *fb, uint32_t fb_w, uint32_t dst_h, const cp_ymap_t *map, uint32_t x, uint32_t y,
                       uint32_t w, uint32_t h, const uint16_t *px, uint16_t c, int swap, uint32_t *y0, uint32_t *y1);
/* The same with a column map: target column x_off + i shows source column xmap[i], i < dst_w. */
uint32_t cp_scale_blit_xy(uint16_t *fb, uint32_t fb_w, uint32_t dst_h, const cp_ymap_t *map, const uint8_t *xmap,
                          uint32_t dst_w, uint32_t x_off, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                          const uint16_t *px, uint16_t c, int swap, uint32_t *y0, uint32_t *y1);

#ifndef CP_SRC_EXTENT
#define CP_SRC_EXTENT 240u    /* the source screen is square: 240 x 240 */
#endif

#ifdef __cplusplus
}
#endif
