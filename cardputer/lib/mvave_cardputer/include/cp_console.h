/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: a line console on the USB serial port. Built-ins: cp (status), heap, keys (the matrix once),
 * shot (the framebuffer as base64 between "SHOT 240 135" and "END", decoded by tools/screenshot.py).
 * Output never blocks: with nobody reading the port, it is dropped. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cp_cmd_fn)(const char *args);

void cp_console_init(void);
void cp_console_poll(void);
int cp_console_register(const char *name, cp_cmd_fn fn);   /* 0 ok, -1 full (16) */
void cp_console_printf(const char *fmt, ...);
void cp_console_line(const char *line);                     /* run one line as if typed (tests) */

#ifdef __cplusplus
}
#endif
