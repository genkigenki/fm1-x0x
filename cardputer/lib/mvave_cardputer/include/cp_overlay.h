/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: what the FM-1's LEDs showed, drawn on the screen (the Cardputer has none). Squeeze and crop:
 * a strip under the firmware's screen, one cell per key (black keys narrower), one per button, the knob's letter.
 * Shrink: the left panel holds the keys, the right one the buttons and the knob. Pure C. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CP_OV_NKEYS 27u
#define CP_OV_NBTN 14u
#define CP_OV_PLAY 10u              /* the button whose cell turns red while recording (X0X's B_PLAY) */
#define CP_OV_LIT 0xFFFFu
#define CP_OV_DIM 0x4208u
#define CP_OV_BTN 0x07E0u
#define CP_OV_REC 0xF800u

void cp_overlay_init(const char *const *enc_names, uint32_t n_enc);   /* a knob shows its name's last digit, else its first letter */
void cp_overlay_leds(uint32_t keys_lit, uint32_t keys_dim, uint32_t buttons_lit, int play_red);
void cp_overlay_knob(int active);      /* -1: none */
void cp_overlay_draw(void);            /* through cp_display_fill_rect; only when something changed */

#ifdef __cplusplus
}
#endif
