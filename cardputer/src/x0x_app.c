/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki)
 * X0X on the M5Stack Cardputer: the app (UI, projects, the screen reader's protocol, the canvas) as one unit, the
 * same sources host/x0x_host.c builds, unchanged. The DSP, sequencer and engine are their own units
 * (platformio.ini, as web/emu/build.sh lists them). Drawing goes to the board layer's 240 x 240 -> 240 x 135
 * screen (mvave_cardputer: cp_display.h); plat.h is implemented in plat_cardputer.c. */
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>                     /* ui.c prints in its X0X_HOST paths: to the USB console here */
#include "cp_display.h"

/* gfx.c puts its canvas in the FM-1's .pool section; the ESP32 has none: as the host build, attributes are
 * dropped from here on (only X0X's own sources follow) */
#define __attribute__(x)

static void lcd_sync(void) {}          /* the FM-1 waits for its SPI DMA here; cp_lcd_blit has already copied */
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c) { cp_lcd_fill(x, y, w, h, c); }
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px) { cp_lcd_blit(x, y, w, h, px); }
#include "gfx.c"

#include "app/plat.h"
#include "app/x0x.h"
#define ANN_NAME "X0X " X0X_VERSION
#include "ann_words.h"
static int ann_wire_send(const uint8_t *p, uint32_t n)   /* no USB MIDI on the Cardputer yet: nobody to tell */
{
    (void)p;
    (void)n;
    return 0;
}
#include "announce.c"
#include "app/project.c"
#include "app/ui.c"

#undef __attribute__

/* the FM-1's boot (main_fm1.c), without its hardware */
void x0x_boot(void)
{
    engine_init(proj.pat, &proj.arr.song, proj.arr.lane, &proj.sound);
    if (project_load() != 0)
        project_defaults();                     /* nothing saved yet (or another format) */
    engine_apply_sound(&proj.sound);
    seq.bpm = (float)proj.set.bpm_x10 / 10.0f;
    seq.accent_q7 = proj.set.accent_q7;
    project_apply_io();
    engine_brk_loops();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui_init();
}

/* one pass of the FM-1's main loop: a UI frame every 16 ms, input in between; 1 = a frame was drawn */
int x0x_tick(uint32_t now_ms)
{
    static uint32_t last_frame;
    static uint8_t last_pat = 0xFF;
    if (seq.ppat[TRK_BRK] != last_pat) {        /* the break's new pattern may name other loops */
        last_pat = seq.ppat[TRK_BRK];
        engine_brk_loops();
    }
    if (now_ms - last_frame >= 16u) {
        last_frame = now_ms;
        ui_frame();
        return 1;
    }
    ui_input_only();
    return 0;
}

/* the audio: what the FM-1's ALNK0 interrupt does for one block */
void x0x_render(int32_t *o, uint32_t n)
{
    engine_render(o, n);
    engine_volume(o, n);
}
void x0x_load(uint32_t pct, uint32_t n) { engine_load(pct, n); }

/* the console's x0x command: what the app sees and does (bring-up) */
#include "cp_console.h"
#include "cp_keys.h"
static void cmd_x0x(const char *args)
{
    (void)args;
    cp_console_printf("playing %d bpm %d keys %07lx buttons %04lx master %lu cpu %lu%%\n", (int)seq.playing,
                      (int)seq.bpm, (unsigned long)plat_keys(), (unsigned long)plat_buttons(),
                      (unsigned long)plat_master(), (unsigned long)plat_cpu_pct());
}
void x0x_console(void) { cp_console_register("x0x", cmd_x0x); }
