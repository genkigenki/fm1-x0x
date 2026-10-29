/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* The default map for a 27-key FM-1 firmware, as X0X uses it (docs/05-keyboard-mapping.md):
 *
 *     `    1    2    3    4    5    6  | 7    8    9    0    -    =    del
 *    SEQ  A#4  C#5  D#5   .   F#5   . | SEL+ ALG+ PRE+ K1+  K2+  K3+  K4+
 *  tab   q    w    e    r    t    y   | u    i    o    p    [    ]    \
 *  HOME  B4   C5   D5   E5   F5   G5  | SEL- ALG- PRE- K1-  K2-  K3-  K4-
 * fn  sh  a    s    d    f    g    h    j    k    l    ;    (quote) enter
 *         OCT- F#3  G#3  A#3   .   C#4  D#4   .   F#4  G#4  OCT+    REC
 * ct op al  z    x    c    v    b    n    m    ,    .    /    space
 * LK -- ++  F3   G3   A3   B3   C4   D4   E4   F4   G4   A4   PLAY
 * fn+:      FX   SEL  ENV  LFO  EDIT GLO  SAVE ARP
 *
 * Buttons in X0X's order (plat.h): FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+. */
#pragma once
#include "cp_keys.h"

static const cp_keymap_t CP_KEYMAP_X0X = {
    .note = {
        {3, 3}, {2, 3}, {3, 4}, {2, 4}, {3, 5}, {2, 5}, {3, 6},          /* F3 F#3 G3 G#3 A3 A#3 B3 */
        {3, 7}, {2, 7}, {3, 8}, {2, 8}, {3, 9},                          /* C4 C#4 D4 D#4 E4 */
        {3, 10}, {2, 10}, {3, 11}, {2, 11}, {3, 12},                     /* F4 F#4 G4 G#4 A4 */
        {0, 1}, {1, 1},                                                  /* A#4 B4 */
        {1, 2}, {0, 2}, {1, 3}, {0, 3}, {1, 4},                          /* C5 C#5 D5 D#5 E5 */
        {1, 5}, {0, 5}, {1, 6},                                          /* F5 F#5 G5 */
    },
    .button = {
        {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8},                  /* fn: FX SEL ENV LFO EDIT GLO */
        {1, 0},                                                          /* HOME: tab */
        {3, 9}, {3, 10},                                                 /* fn: SAVE ARP */
        {0, 0}, {3, 13}, {2, 13},                                        /* SEQ: backtick  PLAY: space  REC: enter */
        {2, 2}, {2, 12},                                                 /* OCT-: a  OCT+: quote */
    },
    .button_fn = {1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0},
    .enc_up = {{0, 7}, {0, 8}, {0, 9}, {0, 10}, {0, 11}, {0, 12}, {0, 13}},
    .enc_down = {{1, 7}, {1, 8}, {1, 9}, {1, 10}, {1, 11}, {1, 12}, {1, 13}},
    .master_up = {3, 2},
    .master_down = {3, 1},
    .layer_lock = {3, 0},
    .fn = {2, 0},
    .shift = {2, 1},
    .enc_fast = 4,
};
