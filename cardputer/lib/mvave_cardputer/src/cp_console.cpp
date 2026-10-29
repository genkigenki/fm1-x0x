// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_heap_caps.h>
#include <stdarg.h>
#include <string.h>
#include "cp_audio.h"
#include "cp_console.h"
#include "cp_display.h"
#include "cp_sys.h"

static struct { const char *name; cp_cmd_fn fn; } cmds[16];
static uint32_t ncmd;
static char line[96];
static uint32_t len;

extern "C" void cp_console_printf(const char *fmt, ...)
{
    char b[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    Serial.print(b);
}

static void cmd_cp(const char *)
{
    cp_console_printf("ms %lu heap %lu min %lu cpu %lu%% peak %lu%% xruns %lu blocks %lu last_us %lu battery %d\n",
                      (unsigned long)cp_ms(), (unsigned long)cp_free_heap(), (unsigned long)cp_min_free_heap(),
                      (unsigned long)cp_audio_cpu_pct(), (unsigned long)cp_audio_peak_pct(),
                      (unsigned long)cp_audio_xruns(), (unsigned long)cp_audio_blocks(),
                      (unsigned long)cp_audio_last_us(), cp_battery_bars());
}

static void cmd_heap(const char *)
{
    cp_console_printf("heap %lu min %lu largest %lu\n", (unsigned long)cp_free_heap(),
                      (unsigned long)cp_min_free_heap(), (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

static void cmd_keys(const char *)
{
    uint8_t m[56] = {0};
    for (const auto &p : M5Cardputer.Keyboard.keyList())
        if (p.x >= 0 && p.x < 14 && p.y >= 0 && p.y < 4)
            m[p.y * 14 + p.x] = 1;
    for (int r = 0; r < 4; r++) {
        char row[16];
        for (int c = 0; c < 14; c++)
            row[c] = m[r * 14 + c] ? '#' : '.';
        row[14] = 0;
        cp_console_printf("%s\n", row);
    }
}

static void cmd_shot(const char *)
{
    static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *p = (const uint8_t *)cp_fb();
    uint32_t n = CP_LCD_W * CP_LCD_H * 2u, i, col = 0;
    char out[80];
    if (!p) {
        cp_console_printf("shot: no framebuffer\n");
        return;
    }
    Serial.setTxTimeoutMs(200);                       // a screenshot is asked for: the PC is reading
    cp_console_printf("SHOT %u %u\n", (unsigned)CP_LCD_W, (unsigned)CP_LCD_H);
    for (i = 0; i < n; i += 3) {
        uint32_t v = (uint32_t)p[i] << 16 | (i + 1 < n ? (uint32_t)p[i + 1] << 8 : 0u) | (i + 2 < n ? p[i + 2] : 0u);
        out[col++] = B64[v >> 18 & 63];
        out[col++] = B64[v >> 12 & 63];
        out[col++] = i + 1 < n ? B64[v >> 6 & 63] : '=';
        out[col++] = i + 2 < n ? B64[v & 63] : '=';
        if (col >= 76) {
            out[col++] = '\n';
            Serial.write((const uint8_t *)out, col);
            col = 0;
        }
    }
    if (col) {
        out[col++] = '\n';
        Serial.write((const uint8_t *)out, col);
    }
    cp_console_printf("END\n");
    Serial.setTxTimeoutMs(0);
}

extern "C" int cp_console_register(const char *name, cp_cmd_fn fn)
{
    if (ncmd >= 16u)
        return -1;
    cmds[ncmd].name = name, cmds[ncmd].fn = fn;
    ncmd++;
    return 0;
}

extern "C" void cp_console_init(void)
{
    Serial.setTxTimeoutMs(0);                         // never wait for a PC that holds the port but does not read
    if (!ncmd) {
        cp_console_register("cp", cmd_cp);
        cp_console_register("heap", cmd_heap);
        cp_console_register("keys", cmd_keys);
        cp_console_register("shot", cmd_shot);
    }
}

extern "C" void cp_console_line(const char *l)
{
    uint32_t i, n;
    for (i = 0; i < ncmd; i++) {
        n = (uint32_t)strlen(cmds[i].name);
        if (!strncmp(l, cmds[i].name, n) && (l[n] == 0 || l[n] == ' ')) {
            cmds[i].fn(l[n] ? l + n + 1 : l + n);
            return;
        }
    }
    if (*l)
        cp_console_printf("? %s (cp heap keys shot", l);
    if (*l) {
        for (i = 4; i < ncmd; i++)
            cp_console_printf(" %s", cmds[i].name);
        cp_console_printf(")\n");
    }
}

extern "C" void cp_console_poll(void)
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == '\r' || c == '\n') {
            line[len] = 0;
            cp_console_line(line);
            len = 0;
        } else if (len + 1 < sizeof line) {
            line[len++] = (char)c;
        }
    }
}
