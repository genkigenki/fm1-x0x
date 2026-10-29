/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki)
 * X0X's platform interface (firmware/src/app/plat.h) on the Cardputer's board layer (mvave_cardputer).
 * What the Cardputer does not have in this version: MIDI, the sample slots, a second render core. */
#include <stddef.h>
#include "app/plat.h"
#include "cp_audio.h"
#include "cp_keys.h"
#include "cp_overlay.h"
#include "cp_store.h"
#include "cp_sys.h"

uint32_t plat_ms(void) { return cp_ms(); }
uint32_t plat_buttons(void) { return cp_keys_buttons(); }
uint32_t plat_keys(void) { return cp_keys_notes() & ((1u << NKEYS) - 1u); }
int32_t plat_enc(int role) { return role >= 0 && role < NE ? cp_enc_take((uint32_t)role) : 0; }
uint32_t plat_master(void) { return cp_master_q12(); }
int plat_battery(void) { return cp_battery_bars(); }

/* the LEDs: the FM-1's key and button lights become the screen's strip (cp_overlay) */
static uint32_t keys_dim;
static int play_red;
void plat_glow(int on) { (void)on; }    /* the unlit buttons' glow: the strip has no glow level */
void plat_play_red(int red) { play_red = red != 0; }
void plat_keys_dim(uint32_t keys) { keys_dim = keys; }
void plat_leds(uint32_t buttons, uint32_t keys)
{
    cp_overlay_leds(keys, keys_dim & ~keys, buttons, play_red);
    cp_overlay_knob(cp_enc_active());
}

int plat_midi_in(uint32_t *pkt)
{
    (void)pkt;
    return 0;
}
void plat_midi_out(uint32_t pkt) { (void)pkt; }

int plat_store_load(uint32_t obj, void *dst, uint32_t max) { return cp_store_load(obj, dst, max); }
int plat_store_save(uint32_t obj, const void *src, uint32_t len) { return cp_store_save(obj, src, len) ? -9 : 0; }
int plat_store_load_prev(uint32_t obj, void *dst, uint32_t max) { return cp_store_load_prev(obj, dst, max); }

int plat_slot_zones(int k)
{
    (void)k;
    return 0;                                     /* no user sample slots: the built-in loops only */
}
const uint8_t *plat_slot(int k, int z, uint32_t *nsamples, uint32_t *rate, char name[9])
{
    (void)k, (void)z, (void)nsamples, (void)rate, (void)name;
    return NULL;
}

uint32_t plat_cpu_pct(void) { return cp_audio_cpu_pct(); }
int plat_cpu2_set(int on) { return on ? -1 : 0; }  /* one render core: the 909 and 808 stay on it */
int plat_cpu2_on(void) { return 0; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { (void)fn, (void)arg; return 0; }
int plat_cpu2_wait(void) { return 0; }
int plat_cpu2_gave_up(void) { return 0; }
uint32_t plat_cpu2_bad(void) { return 0; }
int plat_cpu2_barred(void) { return 0; }
int plat_vdc14_mv(void) { return 0; }
int plat_sysvdd_mv(void) { return 0; }
int plat_last_crash(uint32_t v[5]) { (void)v; return 0; }
uint32_t plat_xruns(void) { return cp_audio_xruns(); }

void plat_perf_init(void) {}
uint32_t plat_cycles(void) { return cp_cycles(); }
int plat_cycles_cpu(void) { return 1; }
uint32_t plat_cycles_hz(void) { return cp_cycles_hz(); }
uint32_t plat_ticks24(void) { return cp_ticks24(); }
uint32_t plat_cpu_peak_pct(void) { return cp_audio_peak_pct(); }
uint32_t plat_irq_stack_used(void) { return 0; }
uint32_t plat_irq_stack_size(void) { return 0; }
int plat_stalls(uint32_t s[3]) { (void)s; return -1; }
void plat_stalls_enable(int on) { (void)on; }
