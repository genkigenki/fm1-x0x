// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
#include <Arduino.h>
#include <M5Cardputer.h>
#include <driver/i2s_std.h>
#include <esp_timer.h>
#include <math.h>
#include "cp_audio.h"

static i2s_chan_handle_t tx;
static TaskHandle_t task;
static cp_render_fn render_fn;
static uint32_t block, rate, trim = 4096u;
static volatile uint32_t cpu_q8, peak, xruns, last_us, blocks;
static volatile int running;
static int32_t blk[2u * CP_AUDIO_MAX_BLOCK];
static int16_t pcm[2u * CP_AUDIO_MAX_BLOCK];
static volatile uint32_t tone_left, out_peak;
static uint32_t tone_ph;

static void audio_task(void *)
{
    const uint32_t budget = block * 1000000u / rate;
    while (running) {
        int64_t t0 = esp_timer_get_time();
        render_fn(blk, block);
        uint32_t us = (uint32_t)(esp_timer_get_time() - t0);
        uint32_t pct = us * 100u / budget;
        last_us = us;
        cpu_q8 = (cpu_q8 * 15u + (us * 256u) / budget) / 16u;
        if (pct > peak)
            peak = pct;
        if (us > budget)
            xruns++;
        blocks++;
        if (tone_left) {                                // the console's test tone, over whatever was rendered
            for (uint32_t i = 0; i < block; i++) {
                int32_t v = (int32_t)(sinf((float)tone_ph * (6.2831853f * 1000.0f) / (float)rate) * 2097152.0f);
                blk[2u * i] = blk[2u * i + 1u] = v;
                tone_ph = (tone_ph + 1u) % rate;
            }
            tone_left = tone_left > block ? tone_left - block : 0u;
        }
        cp_audio_convert(blk, pcm, block, trim);
        for (uint32_t i = 0; i < 2u * block; i += 2u) {   // the level that reaches the speaker
            uint32_t a = (uint32_t)(pcm[i] < 0 ? -pcm[i] : pcm[i]);
            if (a > out_peak)
                out_peak = a;
        }
        size_t done = 0;
        i2s_channel_write(tx, pcm, block * 4u, &done, portMAX_DELAY);
    }
    vTaskDelete(nullptr);
}

extern "C" int cp_audio_start(cp_render_fn render, uint32_t block_frames, uint32_t rate_hz)
{
    if (running || !render || !block_frames || block_frames > CP_AUDIO_MAX_BLOCK)
        return -1;
    M5.Speaker.end();                                   // M5Unified's speaker would hold the same pins
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    cc.dma_desc_num = 4;
    cc.dma_frame_num = block_frames;
    cc.auto_clear = true;                               // an underrun plays silence, not the last block again
    if (i2s_new_channel(&cc, &tx, nullptr) != ESP_OK)
        return -2;
    i2s_std_config_t sc = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate_hz),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = (gpio_num_t)CP_I2S_BCK, .ws = (gpio_num_t)CP_I2S_WS,
                     .dout = (gpio_num_t)CP_I2S_DOUT, .din = I2S_GPIO_UNUSED,
                     .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false}},
    };
    if (i2s_channel_init_std_mode(tx, &sc) != ESP_OK || i2s_channel_enable(tx) != ESP_OK) {
        i2s_del_channel(tx);
        tx = nullptr;
        return -3;
    }
    render_fn = render, block = block_frames, rate = rate_hz;
    cpu_q8 = peak = xruns = last_us = blocks = 0;
    running = 1;
    if (xTaskCreatePinnedToCore(audio_task, "cp_audio", 8192, nullptr, configMAX_PRIORITIES - 2, &task, 1) != pdPASS) {
        running = 0;
        return -4;
    }
    return 0;
}

extern "C" void cp_audio_stop(void)
{
    if (!running)
        return;
    running = 0;
    delay(3u * block * 1000u / rate + 20u);              // the task finishes its block and deletes itself
    i2s_channel_disable(tx);
    i2s_del_channel(tx);
    tx = nullptr;
}

extern "C" void cp_audio_set_trim_q12(uint32_t q12) { trim = q12 > 65536u ? 65536u : q12; }   /* up to x16, as M5Unified drives this speaker */
extern "C" uint32_t cp_audio_trim_q12(void) { return trim; }
extern "C" uint32_t cp_audio_cpu_pct(void) { return (cpu_q8 * 100u) >> 8; }
extern "C" uint32_t cp_audio_peak_pct(void)
{
    uint32_t p = peak;
    peak = 0;
    return p;
}
extern "C" uint32_t cp_audio_xruns(void) { return xruns; }
extern "C" uint32_t cp_audio_last_us(void) { return last_us; }
extern "C" uint32_t cp_audio_blocks(void) { return blocks; }

extern "C" void cp_audio_test_tone(uint32_t ms) { tone_left = ms * (rate ? rate : 44100u) / 1000u; }
extern "C" uint32_t cp_audio_out_peak(void)
{
    uint32_t p = out_peak;
    out_peak = 0;
    return p;
}
