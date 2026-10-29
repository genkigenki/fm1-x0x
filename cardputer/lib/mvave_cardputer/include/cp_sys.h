/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: clocks, battery, the boot-loop guard, the watchdog. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

uint32_t cp_ms(void);
uint32_t cp_ticks24(void);             /* 24 MHz ticks, as the FM-1's TIMER4 */
uint32_t cp_cycles(void);              /* the CPU's cycle counter */
uint32_t cp_cycles_hz(void);
int cp_battery_bars(void);             /* 0..3; 4 = charging / powered */
void cp_reboot(void);
int cp_bootguard_begin(void);          /* boots that did not reach healthy, before this one (NVS "cp"/"boots") */
void cp_bootguard_healthy(void);
void cp_wdt_feed(void);
uint32_t cp_free_heap(void);
uint32_t cp_min_free_heap(void);

#ifdef __cplusplus
}
#endif
