// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
// X0X on the M5Stack Cardputer: boot, the audio task and the main loop, after main_fm1.c.
// The render runs in a task on the same core as this loop, at a higher priority: like the FM-1's audio
// interrupt it preempts the main loop and is never preempted by it, so X0X's shared state needs nothing new.
#include <M5Cardputer.h>
#include "cp_audio.h"
#include "cp_console.h"
#include "cp_display.h"
#include "cp_keys.h"
#include "cp_keymap_x0x.h"
#include "cp_overlay.h"
#include "cp_store.h"
#include "cp_sys.h"

extern "C" {
void x0x_boot(void);
int x0x_tick(uint32_t now_ms);
void x0x_render(int32_t *o, uint32_t n);
void x0x_load(uint32_t pct, uint32_t n);
void x0x_console(void);
}

#ifndef X0X_CP_SCREEN
#define X0X_CP_SCREEN CP_SCREEN_SQUEEZE
#endif
#ifndef X0X_CP_TRIM_Q12
#define X0X_CP_TRIM_Q12 32768u                   // x8: the FM-1 is line level, this speaker needs gain (M5Unified: x16); console: gain N
#endif
#define BLOCK 256u
#define RATE 44100u

static const char *const KNOBS[7] = {"SELECT", "ALGORITHM", "PRESETS", "KNOB 1", "KNOB 2", "KNOB 3", "KNOB 4"};
static int safe_mode;
static uint32_t frame_sum, frames, frame_max, loops;

static void cmd_perf(const char *)
{
    cp_console_printf("ui frames %lu avg %lu us max %lu us, loops %lu (since the last perf)\n", (unsigned long)frames,
                      (unsigned long)(frames ? frame_sum / frames : 0), (unsigned long)frame_max, (unsigned long)loops);
    frame_sum = frames = frame_max = loops = 0;
}

static void render(int32_t *o, uint32_t n)
{
    x0x_render(o, n);
    x0x_load(cp_audio_last_us() * 100u / (BLOCK * 1000000u / RATE), n);   // the last block's load: X0X's quality guard
}

void setup()
{
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);
    cp_console_init();
    x0x_console();
    cp_console_register("perf", cmd_perf);
    safe_mode = cp_bootguard_begin() >= 2;          // two boots that never got healthy: no audio this time
    cp_display_init(X0X_CP_SCREEN, CP_STRIP_H);
    cp_overlay_init(KNOBS, 7);
    cp_keys_init(&CP_KEYMAP_X0X);
    cp_store_init("x0x");
    x0x_boot();
    if (!safe_mode) {
        cp_audio_set_trim_q12(X0X_CP_TRIM_Q12);
        cp_audio_start(render, BLOCK, RATE);
    }
}

void loop()
{
    static uint32_t last_ms, healthy;
    uint32_t now = cp_ms();
    loops++;
    cp_keys_scan();
    if (now != last_ms) {
        last_ms = now;
        uint32_t t0 = micros();
        if (x0x_tick(now)) {
            cp_lcd_sync();
            uint32_t us = micros() - t0;                // one UI frame, drawing included (console: perf)
            frame_sum += us, frames++;
            if (us > frame_max)
                frame_max = us;
        }
    }
    cp_console_poll();
    if (!healthy && now > 10000u) {
        healthy = 1;
        cp_bootguard_healthy();
    }
    vTaskDelay(1);                                   // the idle task and the watchdog get their turn
}
