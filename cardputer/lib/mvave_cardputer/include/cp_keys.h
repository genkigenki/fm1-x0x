/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the Cardputer's 56 keys as an FM-1 panel: note keys, buttons, encoders, master. The logic
 * (cp_keys_feed) is pure C; cp_keys_scan reads the matrix from M5Cardputer and feeds it.
 * A key's role is decided when it goes down: with the fn layer on (fn held, or locked), a key that has an fn
 * button is that button; otherwise its normal role. A note keeps sounding until its key is released. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CP_KB_ROWS 4u
#define CP_KB_COLS 14u
#define CP_NOTES 27u
#define CP_BUTTONS 14u
#define CP_ENCS 7u

typedef struct { uint8_t row, col; } cp_keypos_t;    /* row 0..3, col 0..13; {0xFF, 0xFF} = none */

typedef struct {
    cp_keypos_t note[CP_NOTES];
    cp_keypos_t button[CP_BUTTONS];
    uint8_t button_fn[CP_BUTTONS];                    /* 1: this button needs the fn layer */
    cp_keypos_t enc_up[CP_ENCS], enc_down[CP_ENCS];   /* one key column per knob: upper +1, lower -1 */
    cp_keypos_t master_up, master_down, layer_lock, fn, shift;
    uint8_t enc_fast;                                 /* detents per press with shift */
} cp_keymap_t;

void cp_keys_init(const cp_keymap_t *map);
void cp_keys_scan(void);                              /* hardware: call every millisecond or so */
void cp_keys_feed(const uint8_t *pressed, uint32_t now_ms);   /* pressed[row * 14 + col] */
uint32_t cp_keys_notes(void);                         /* bit k = FM-1 key k (0 = F3) */
uint32_t cp_keys_buttons(void);                       /* bit b = button b */
int32_t cp_enc_take(uint32_t role);                   /* detents since the last call */
int cp_enc_active(void);                              /* the last knob turned, -1 none */
uint32_t cp_master_q12(void);                         /* 0..4096 */
int cp_fn_held(void);                                 /* the fn layer is on */

#define CP_REPEAT_DELAY_MS 400u
#define CP_REPEAT_EVERY_MS 60u
#define CP_MASTER_STEP 256u
#define CP_MASTER_START 3072u

#ifdef __cplusplus
}
#endif
