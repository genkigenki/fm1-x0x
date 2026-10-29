/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include <string.h>
#include "cp_keys.h"

#define NK (CP_KB_ROWS * CP_KB_COLS)
enum { R_NONE, R_NOTE, R_BUTTON, R_ENC_UP, R_ENC_DOWN, R_MASTER_UP, R_MASTER_DOWN, R_LOCK, R_FN, R_SHIFT };

static struct {
    int8_t note[NK], btn[NK], btn_fn[NK], enc[NK];
    uint8_t fixed[NK];                 /* roles that do not depend on the layer */
    uint8_t enc_fast;
} map;
static struct {
    uint8_t down[NK], role[NK], idx[NK];
    uint32_t t_down[NK], emitted[NK];
    uint32_t notes, buttons, master;
    int32_t enc[CP_ENCS];
    int active, lock, fn, shift;
} st;
static uint8_t ghost[NK];          /* reported down, taken as a phantom of three held keys */

static int pos(cp_keypos_t p)
{
    return p.row < CP_KB_ROWS && p.col < CP_KB_COLS ? p.row * (int)CP_KB_COLS + p.col : -1;
}

void cp_keys_init(const cp_keymap_t *m)
{
    uint32_t i;
    int k;
    memset(&map, 0, sizeof map);
    memset(&st, 0, sizeof st);
    memset(ghost, 0, sizeof ghost);
    memset(map.note, -1, sizeof map.note);
    memset(map.btn, -1, sizeof map.btn);
    memset(map.btn_fn, -1, sizeof map.btn_fn);
    memset(map.enc, -1, sizeof map.enc);
    for (i = 0; i < CP_NOTES; i++)
        if ((k = pos(m->note[i])) >= 0)
            map.note[k] = (int8_t)i;
    for (i = 0; i < CP_BUTTONS; i++)
        if ((k = pos(m->button[i])) >= 0) {
            if (m->button_fn[i])
                map.btn_fn[k] = (int8_t)i;
            else
                map.btn[k] = (int8_t)i;
        }
    for (i = 0; i < CP_ENCS; i++) {
        if ((k = pos(m->enc_up[i])) >= 0)
            map.fixed[k] = R_ENC_UP, map.enc[k] = (int8_t)i;
        if ((k = pos(m->enc_down[i])) >= 0)
            map.fixed[k] = R_ENC_DOWN, map.enc[k] = (int8_t)i;
    }
    if ((k = pos(m->master_up)) >= 0)
        map.fixed[k] = R_MASTER_UP;
    if ((k = pos(m->master_down)) >= 0)
        map.fixed[k] = R_MASTER_DOWN;
    if ((k = pos(m->layer_lock)) >= 0)
        map.fixed[k] = R_LOCK;
    if ((k = pos(m->fn)) >= 0)
        map.fixed[k] = R_FN;
    if ((k = pos(m->shift)) >= 0)
        map.fixed[k] = R_SHIFT;
    map.enc_fast = m->enc_fast ? m->enc_fast : 1u;
    st.master = CP_MASTER_START;
    st.active = -1;
}

static void step(uint32_t k)            /* one detent, or one master step, for the key's role */
{
    int32_t n = st.shift ? map.enc_fast : 1;
    uint8_t r = st.role[k];
    if (r == R_ENC_UP || r == R_ENC_DOWN) {
        st.enc[st.idx[k]] += r == R_ENC_UP ? n : -n;
        st.active = st.idx[k];
    } else if (r == R_MASTER_UP) {
        st.master = st.master + CP_MASTER_STEP > 4096u ? 4096u : st.master + CP_MASTER_STEP;
    } else if (r == R_MASTER_DOWN) {
        st.master = st.master < CP_MASTER_STEP ? 0u : st.master - CP_MASTER_STEP;
    }
}

static void key_down(uint32_t k, uint32_t now)
{
    int layer = st.fn || st.lock;
    st.down[k] = 1;
    st.t_down[k] = now;
    st.emitted[k] = 0;
    st.role[k] = R_NONE;
    if (layer && map.btn_fn[k] >= 0)
        st.role[k] = R_BUTTON, st.idx[k] = (uint8_t)map.btn_fn[k];
    else if (map.note[k] >= 0)
        st.role[k] = R_NOTE, st.idx[k] = (uint8_t)map.note[k];
    else if (map.btn[k] >= 0)
        st.role[k] = R_BUTTON, st.idx[k] = (uint8_t)map.btn[k];
    else if (map.fixed[k]) {
        st.role[k] = map.fixed[k];
        st.idx[k] = map.enc[k] >= 0 ? (uint8_t)map.enc[k] : 0u;
    }
    switch (st.role[k]) {
    case R_NOTE: st.notes |= 1u << st.idx[k]; break;
    case R_BUTTON: st.buttons |= 1u << st.idx[k]; break;
    case R_LOCK: st.lock = !st.lock; break;
    case R_FN: st.fn = 1; break;
    case R_SHIFT: st.shift = 1; break;
    default: step(k); break;
    }
}

static void key_up(uint32_t k)
{
    st.down[k] = 0;
    switch (st.role[k]) {
    case R_NOTE: st.notes &= ~(1u << st.idx[k]); break;
    case R_BUTTON: st.buttons &= ~(1u << st.idx[k]); break;
    case R_FN: st.fn = 0; break;
    case R_SHIFT: st.shift = 0; break;
    default: break;
    }
    st.role[k] = R_NONE;
}

static void key_held(uint32_t k, uint32_t now)
{
    uint8_t r = st.role[k];
    uint32_t held = now - st.t_down[k], due;
    if (r != R_ENC_UP && r != R_ENC_DOWN && r != R_MASTER_UP && r != R_MASTER_DOWN)
        return;
    if (held < CP_REPEAT_DELAY_MS)
        return;
    due = (held - CP_REPEAT_DELAY_MS) / CP_REPEAT_EVERY_MS + 1u;   /* by elapsed time, not by calls */
    while (st.emitted[k] < due) {
        st.emitted[k]++;
        step(k);
    }
}

/* The Cardputer's keys sit on 8 scan lines x 7 inputs without diodes (M5Cardputer IOMatrix): row r, column c is
 * line (3 - r) + (c even ? 4 : 0), input c / 2. Three keys held on three corners of a rectangle read the fourth
 * corner as down too. A key that appears while the three other corners are already held is taken as that phantom
 * and ignored until the matrix reports it up (a real fourth key of such a rectangle is lost: docs/05). */
static uint32_t phys_line(uint32_t k) { return (3u - k / CP_KB_COLS) + ((k % CP_KB_COLS) % 2u ? 0u : 4u); }
static uint32_t phys_in(uint32_t k) { return (k % CP_KB_COLS) / 2u; }

static int is_ghost(uint32_t k)
{
    uint32_t a, b, l = phys_line(k), j = phys_in(k);
    for (a = 0; a < NK; a++) {                       /* a: same line, b: same input, and their corner */
        if (a == k || !st.down[a] || phys_line(a) != l)
            continue;
        for (b = 0; b < NK; b++) {
            uint32_t c;
            if (b == k || !st.down[b] || phys_in(b) != j)
                continue;
            for (c = 0; c < NK; c++)
                if (st.down[c] && phys_line(c) == phys_line(b) && phys_in(c) == phys_in(a))
                    return 1;
        }
    }
    return 0;
}

void cp_keys_feed(const uint8_t *pressed, uint32_t now)
{
    uint32_t k;
    for (k = 0; k < NK; k++) {
        if (!pressed[k])
            ghost[k] = 0;
        else if (!st.down[k] && !ghost[k] && is_ghost(k))
            ghost[k] = 1;
    }
    /* fn and shift first, so a key that goes down in the same scan already sees them */
    for (k = 0; k < NK; k++)
        if (pressed[k] && !ghost[k] && !st.down[k] && (map.fixed[k] == R_FN || map.fixed[k] == R_SHIFT))
            key_down(k, now);
    for (k = 0; k < NK; k++) {
        if (ghost[k])
            continue;
        if (pressed[k] && !st.down[k])
            key_down(k, now);
        else if (!pressed[k] && st.down[k])
            key_up(k);
        else if (pressed[k])
            key_held(k, now);
    }
}

uint32_t cp_keys_notes(void) { return st.notes; }
uint32_t cp_keys_buttons(void) { return st.buttons; }
int32_t cp_enc_take(uint32_t role)
{
    int32_t v;
    if (role >= CP_ENCS)
        return 0;
    v = st.enc[role];
    st.enc[role] = 0;
    return v;
}
int cp_enc_active(void) { return st.active; }
uint32_t cp_master_q12(void) { return st.master; }
int cp_fn_held(void) { return st.fn || st.lock; }
