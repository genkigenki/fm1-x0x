// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <xtensa/core-macros.h>
#include "cp_sys.h"

extern "C" uint32_t cp_ms(void) { return millis(); }
extern "C" uint32_t cp_ticks24(void) { return (uint32_t)(esp_timer_get_time() * 24); }
extern "C" uint32_t cp_cycles(void) { return XTHAL_GET_CCOUNT(); }
extern "C" uint32_t cp_cycles_hz(void) { return (uint32_t)getCpuFrequencyMhz() * 1000000u; }

extern "C" int cp_battery_bars(void)
{
    int lvl = M5.Power.getBatteryLevel();
    if (M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging)
        return 4;
    if (lvl < 0)
        return -1;
    return lvl >= 70 ? 3 : lvl >= 35 ? 2 : lvl >= 10 ? 1 : 0;
}

extern "C" void cp_reboot(void) { ESP.restart(); }

static int guard_open(nvs_handle_t *h)
{
    nvs_flash_init();
    return nvs_open("cp", NVS_READWRITE, h) == ESP_OK;
}

extern "C" int cp_bootguard_begin(void)
{
    nvs_handle_t h;
    uint8_t n = 0;
    esp_reset_reason_t why = esp_reset_reason();
    /* only a crash counts: a power-on, a reset button, a USB reset from a flasher or the Launcher's restart is
     * not a failed boot (a flasher resets the chip many times in a row) */
    int crash = why == ESP_RST_PANIC || why == ESP_RST_INT_WDT || why == ESP_RST_TASK_WDT || why == ESP_RST_WDT ||
                why == ESP_RST_BROWNOUT;
    if (!guard_open(&h))
        return 0;
    nvs_get_u8(h, "boots", &n);
    if (!crash)
        n = 0;
    nvs_set_u8(h, "boots", (uint8_t)(n < 255 ? n + 1 : 255));
    nvs_commit(h);
    nvs_close(h);
    return n;
}

extern "C" void cp_bootguard_healthy(void)
{
    nvs_handle_t h;
    if (!guard_open(&h))
        return;
    nvs_set_u8(h, "boots", 0);
    nvs_commit(h);
    nvs_close(h);
}

extern "C" void cp_wdt_feed(void) { delay(0); }
extern "C" uint32_t cp_free_heap(void) { return ESP.getFreeHeap(); }
extern "C" uint32_t cp_min_free_heap(void) { return ESP.getMinFreeHeap(); }
