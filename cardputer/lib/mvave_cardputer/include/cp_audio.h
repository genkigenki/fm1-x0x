/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the firmware's audio render on the Cardputer's speaker (NS4168, I2S: BCLK 41, WS 43,
 * DOUT 42). A task on core 1 calls the render for one block at a time, in the FM-1's format (interleaved stereo
 * int32, full scale +-8,388,608), mixes it to mono, applies the trim, saturates to 16 bits and writes it to I2S.
 * It measures how long each render takes against the block's time: cpu %, peak %, xruns. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cp_render_fn)(int32_t *out, uint32_t frames);

int cp_audio_start(cp_render_fn render, uint32_t block_frames, uint32_t rate_hz);   /* 0 ok */
void cp_audio_stop(void);
void cp_audio_set_trim_q12(uint32_t q12);      /* 4096 = 1.0 (default), up to 65536 = x16 */
uint32_t cp_audio_trim_q12(void);
uint32_t cp_audio_cpu_pct(void);               /* smoothed */
uint32_t cp_audio_peak_pct(void);              /* the highest since the last call */
uint32_t cp_audio_xruns(void);                 /* renders longer than their block */
uint32_t cp_audio_last_us(void);
uint32_t cp_audio_blocks(void);
void cp_audio_test_tone(uint32_t ms);           /* a 1 kHz tone at -12 dBFS over the render (the console's tone) */
uint32_t cp_audio_out_peak(void);              /* the loudest 16-bit sample sent to the speaker since the last call */

/* pure: in = interleaved stereo int32 (+-8,388,608), out = interleaved stereo int16 (the mono mix twice) */
void cp_audio_convert(const int32_t *in, int16_t *out, uint32_t frames, uint32_t trim_q12);

#define CP_AUDIO_MAX_BLOCK 512u
#define CP_I2S_BCK 41
#define CP_I2S_WS 43
#define CP_I2S_DOUT 42

#ifdef __cplusplus
}
#endif
