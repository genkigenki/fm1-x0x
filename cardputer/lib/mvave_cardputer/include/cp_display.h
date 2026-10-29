/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the firmware's 240 x 240 screen on the Cardputer's 240 x 135 panel. The firmware calls
 * cp_lcd_fill / cp_lcd_blit in its own coordinates and cp_lcd_sync at the end of a frame. There is no framebuffer
 * (the RAM goes to the firmware): each blit is scaled row band by row band and pushed straight to the panel, which
 * keeps the picture; cp_lcd_sync draws the overlay (the LED strip) when it changed. */
#pragma once
#include <stdint.h>
#include "cp_config.h"
#ifdef __cplusplus
extern "C" {
#endif

enum { CP_SCREEN_SQUEEZE = 0, CP_SCREEN_SHRINK = 1, CP_SCREEN_CROP = 2 };

int cp_display_init(int strategy, uint32_t strip_h);   /* 0 ok */
void cp_lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c);      /* c: RGB565, CPU order */
void cp_lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px);   /* CP_BLIT_SWAP order */
void cp_lcd_sync(void);
uint32_t cp_fb_ui_h(void);             /* panel rows the firmware's screen takes (squeeze, crop); 135 for shrink */
int cp_display_strategy(void);
void cp_display_crop_dy(int32_t dy);   /* crop: the first source row shown (clamped); redraw the firmware after */
void cp_display_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c);   /* panel coordinates */
void cp_overlay_draw(void);            /* weak no-op here; cp_overlay.c defines it */
uint32_t cp_display_last_blit_us(void);

#define CP_BAND_ROWS 16u               /* panel rows scaled and pushed at a time: 240 x 16 x 2 = 7,680 B */

#ifdef __cplusplus
}
#endif
