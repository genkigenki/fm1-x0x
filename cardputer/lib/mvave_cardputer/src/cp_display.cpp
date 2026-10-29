// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
#include "cp_display.h"
#include "cp_scale.h"
#include <M5Cardputer.h>

static uint16_t band[CP_LCD_W * CP_BAND_ROWS];
static cp_ymap_t ymap[CP_LCD_H];
static uint8_t xmap[CP_LCD_H];
static uint8_t cols[CP_LCD_W];
static int strategy;
static uint32_t ui_h, crop_dy, blit_us;

extern "C" __attribute__((weak)) void cp_overlay_draw(void) {}

static void build_maps(void)
{
    if (strategy == CP_SCREEN_SHRINK) {
        cp_ymap_build(ymap, CP_SRC_H, CP_LCD_H);
        cp_xmap_build(xmap, CP_SRC_W, CP_LCD_H);
    } else if (strategy == CP_SCREEN_CROP) {
        cp_ymap_crop(ymap, ui_h, crop_dy);
    } else {
        cp_ymap_build(ymap, CP_SRC_H, ui_h);
    }
}

extern "C" int cp_display_init(int s, uint32_t strip_h)
{
    strategy = s;
    ui_h = s == CP_SCREEN_SHRINK ? CP_LCD_H : CP_LCD_H - (strip_h < CP_LCD_H ? strip_h : 0u);
    build_maps();
    M5.Display.setRotation(1);
    M5.Display.fillScreen(TFT_BLACK);
    return 0;
}

static void push(uint32_t x, uint32_t t0, uint32_t w, uint32_t rows)
{
    M5.Display.pushImage((int32_t)x, (int32_t)t0, (int32_t)w, (int32_t)rows, (const lgfx::rgb565_t *)band);
}

static void draw(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px, uint16_t c, int swap)
{
    uint32_t t0 = micros(), x0, n, t, rows = 0, band_t0 = 0;
    const uint8_t *cl = 0;
    if (x >= CP_SRC_W || y >= CP_SRC_H || !w || !h)
        return;
    if (strategy == CP_SCREEN_SHRINK) {               // the panel columns whose source column is in the blit
        uint32_t i, a = CP_LCD_H, b = 0, xe = x + w > CP_SRC_W ? CP_SRC_W : x + w;
        for (i = 0; i < CP_LCD_H; i++)
            if (xmap[i] >= x && xmap[i] < xe) {
                if (a == CP_LCD_H)
                    a = i;
                b = i;
            }
        if (a == CP_LCD_H)
            return;
        x0 = (CP_LCD_W - CP_LCD_H) / 2u + a;
        n = b - a + 1u;
        for (i = 0; i < n; i++)
            cols[i] = xmap[a + i];
        cl = cols;
    } else {
        x0 = x;
        n = (x + w > CP_SRC_W ? CP_SRC_W - x : w);
    }
    M5.Display.startWrite();
    for (t = 0; t < ui_h; t++) {
        uint16_t *row = band + rows * n;
        if (!cp_scale_row(row, &ymap[t], x, y, w, h, px, c, swap, cl, n)) {
            if (rows) {                                // a gap: push what the band holds
                push(x0, band_t0, n, rows);
                rows = 0;
            }
            continue;
        }
        if (!rows)
            band_t0 = t;
        if (++rows == CP_BAND_ROWS) {
            push(x0, band_t0, n, rows);
            rows = 0;
        }
    }
    if (rows)
        push(x0, band_t0, n, rows);
    M5.Display.endWrite();
    blit_us = micros() - t0;
}

extern "C" void cp_lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { draw(x, y, w, h, 0, c, 0); }
extern "C" void cp_lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    draw(x, y, w, h, px, 0, CP_BLIT_SWAP);
}
extern "C" void cp_lcd_sync(void) { cp_overlay_draw(); }

extern "C" void cp_display_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    M5.Display.fillRect((int32_t)x, (int32_t)y, (int32_t)w, (int32_t)h, (uint16_t)c);
}

extern "C" uint32_t cp_fb_ui_h(void) { return ui_h; }
extern "C" int cp_display_strategy(void) { return strategy; }
extern "C" uint32_t cp_display_last_blit_us(void) { return blit_us; }
extern "C" void cp_display_crop_dy(int32_t dy)
{
    int32_t max = (int32_t)(CP_SRC_H - ui_h);
    crop_dy = (uint32_t)(dy < 0 ? 0 : dy > max ? max : dy);
    if (strategy == CP_SCREEN_CROP)
        build_maps();
}
