/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: board constants. A port overrides them with -D in its platformio.ini. */
#pragma once
#ifndef CP_LCD_W
#define CP_LCD_W 240u          /* the Cardputer's ST7789V2 in landscape */
#endif
#ifndef CP_LCD_H
#define CP_LCD_H 135u
#endif
#ifndef CP_SRC_W
#define CP_SRC_W 240u          /* the firmware's screen (FM-1, SMK-37) */
#endif
#ifndef CP_SRC_H
#define CP_SRC_H 240u
#endif
#ifndef CP_STRIP_H
#define CP_STRIP_H 7u          /* rows under the squeezed screen for the LED strip */
#endif
#ifndef CP_BLIT_SWAP
#define CP_BLIT_SWAP 1         /* the firmware's blits are RGB565 high byte first (the panel's order) */
#endif
#ifndef CP_AUDIO_RATE
#define CP_AUDIO_RATE 44100u
#endif
#ifndef CP_AUDIO_BLOCK
#define CP_AUDIO_BLOCK 256u
#endif
