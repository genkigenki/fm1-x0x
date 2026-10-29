/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the firmware's 240 x 240 screen on the Cardputer's 240 x 135 panel, through a framebuffer.
 * The firmware calls cp_lcd_fill / cp_lcd_blit in its own coordinates, and cp_lcd_sync at the end of a frame:
 * the overlay is drawn and the dirty rows are pushed to the panel. */
#pragma once
#include <stdint.h>
#include "cp_config.h"
#ifdef __cplusplus
extern "C" {
#endif

enum { CP_SCREEN_SQUEEZE = 0, CP_SCREEN_SHRINK = 1, CP_SCREEN_CROP = 2 };

int cp_display_init(int strategy, uint32_t strip_h);   /* 0 ok, -1 no memory */
void cp_lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c);      /* c: RGB565, CPU order */
void cp_lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px);   /* CP_BLIT_SWAP order */
void cp_lcd_sync(void);
uint16_t *cp_fb(void);                 /* 240 x 135, RGB565 CPU order */
uint32_t cp_fb_ui_h(void);             /* rows the firmware's screen takes (squeeze, crop); 135 for shrink */
int cp_display_strategy(void);
void cp_display_crop_dy(int32_t dy);   /* crop: the first source row shown (clamped) */
void cp_display_mark_dirty(uint32_t y0, uint32_t y1);
void cp_overlay_draw(void);            /* weak no-op here; cp_overlay.cpp defines it */
uint32_t cp_display_last_sync_us(void);

#ifdef __cplusplus
}
#endif
