// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
#include "cp_display.h"
#include "cp_scale.h"
#include <M5Cardputer.h>
#include <esp_heap_caps.h>

static uint16_t *fb;
static cp_ymap_t ymap[CP_LCD_H];
static uint8_t xmap[CP_LCD_H];
static int strategy;
static uint32_t ui_h, dirty0 = 1, dirty1, crop_dy, sync_us;

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
    if (!fb)
        fb = (uint16_t *)heap_caps_calloc(CP_LCD_W * CP_LCD_H, 2, MALLOC_CAP_8BIT);
    if (!fb)
        return -1;
    strategy = s;
    ui_h = s == CP_SCREEN_SHRINK ? CP_LCD_H : CP_LCD_H - (strip_h < CP_LCD_H ? strip_h : 0u);
    build_maps();
    M5.Display.setRotation(1);
    M5.Display.fillScreen(TFT_BLACK);
    cp_display_mark_dirty(0, CP_LCD_H - 1u);
    return 0;
}

extern "C" void cp_display_mark_dirty(uint32_t y0, uint32_t y1)
{
    if (y1 >= CP_LCD_H)
        y1 = CP_LCD_H - 1u;
    if (dirty0 > dirty1) {
        dirty0 = y0, dirty1 = y1;
        return;
    }
    if (y0 < dirty0)
        dirty0 = y0;
    if (y1 > dirty1)
        dirty1 = y1;
}

static void draw(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px, uint16_t c, int swap)
{
    uint32_t y0 = 0, y1 = 0, n;
    if (!fb)
        return;
    if (strategy == CP_SCREEN_SHRINK)
        n = cp_scale_blit_xy(fb, CP_LCD_W, ui_h, ymap, xmap, CP_LCD_H, (CP_LCD_W - CP_LCD_H) / 2u, x, y, w, h, px,
                             c, swap, &y0, &y1);
    else
        n = cp_scale_blit(fb, CP_LCD_W, ui_h, ymap, x, y, w, h, px, c, swap, &y0, &y1);
    if (n)
        cp_display_mark_dirty(y0, y1);
}

extern "C" void cp_lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { draw(x, y, w, h, 0, c, 0); }
extern "C" void cp_lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    draw(x, y, w, h, px, 0, CP_BLIT_SWAP);
}

extern "C" void cp_lcd_sync(void)
{
    uint32_t t0 = micros();
    if (!fb)
        return;
    cp_overlay_draw();
    if (dirty0 <= dirty1) {
        M5.Display.pushImage(0, (int32_t)dirty0, CP_LCD_W, (int32_t)(dirty1 - dirty0 + 1u),
                             (const lgfx::rgb565_t *)(fb + dirty0 * CP_LCD_W));
        dirty0 = 1, dirty1 = 0;
    }
    sync_us = micros() - t0;
}

extern "C" uint16_t *cp_fb(void) { return fb; }
extern "C" uint32_t cp_fb_ui_h(void) { return ui_h; }
extern "C" int cp_display_strategy(void) { return strategy; }
extern "C" uint32_t cp_display_last_sync_us(void) { return sync_us; }
extern "C" void cp_display_crop_dy(int32_t dy)
{
    int32_t max = (int32_t)(CP_SRC_H - ui_h);
    crop_dy = (uint32_t)(dy < 0 ? 0 : dy > max ? max : dy);
    if (strategy == CP_SCREEN_CROP)
        build_maps();
}
