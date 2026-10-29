// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
// The Cardputer's matrix into cp_keys_feed: M5Cardputer.update() scans it, keyList() holds the keys down as
// (x = column 0..13, y = row 0..3), the same coordinates as M5Cardputer's _key_value_map[y][x].
#include <M5Cardputer.h>
#include <string.h>
#include "cp_keys.h"

extern "C" void cp_keys_scan(void)
{
    static uint8_t pressed[CP_KB_ROWS * CP_KB_COLS];
    M5Cardputer.update();
    memset(pressed, 0, sizeof pressed);
    for (const auto &p : M5Cardputer.Keyboard.keyList())
        if (p.x >= 0 && p.x < (int)CP_KB_COLS && p.y >= 0 && p.y < (int)CP_KB_ROWS)
            pressed[p.y * CP_KB_COLS + p.x] = 1;
    cp_keys_feed(pressed, millis());
}
