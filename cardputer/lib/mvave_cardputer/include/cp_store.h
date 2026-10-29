/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
/* mvave-to-cardputer: the firmware's stored objects (settings, patterns, projects) with Felucca's A/B semantics:
 * every object has two copies; a save overwrites the older one; a load returns the newest whose CRC is good, so a
 * save cut off by a power loss loses nothing. The copies are blobs in a key-value backend: NVS on the device
 * (CP_STORE_NVS, the namespace the port names), RAM in the host tests. */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CP_STORE_MAX 3840u             /* payload bytes per object, as Felucca's storage */

typedef struct {
    int (*get)(const char *key, void *dst, uint32_t *len);   /* 0 ok (*len = bytes), -1 none, -2 too small */
    int (*set)(const char *key, const void *src, uint32_t len);
    int (*erase_all)(void);
    int (*open)(const char *ns);       /* may be NULL */
} cp_store_backend_t;

extern const cp_store_backend_t CP_STORE_NVS;

int cp_store_init(const char *ns);                                   /* NVS */
int cp_store_init_with(const char *ns, const cp_store_backend_t *backend);
int cp_store_load(uint32_t obj, void *dst, uint32_t max);           /* bytes; -1 none; -2 both copies bad or dst too small */
int cp_store_save(uint32_t obj, const void *src, uint32_t len);     /* 0 ok; -3 too big; -9 backend error */
int cp_store_load_prev(uint32_t obj, void *dst, uint32_t max);      /* the copy before the newest */
int cp_store_erase_all(void);
uint32_t cp_crc32(const void *p, uint32_t n);                       /* zlib CRC-32 */
void cp_store_test_corrupt_newest(uint32_t obj);                    /* tests: flip a payload byte of the newest copy */

#ifdef __cplusplus
}
#endif
