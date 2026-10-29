/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include <string.h>
#include "cp_store.h"

typedef struct {
    uint32_t gen, len, crc;            /* crc over the payload */
} hdr_t;

static const cp_store_backend_t *be;
static uint8_t scratch[sizeof(hdr_t) + CP_STORE_MAX];

uint32_t cp_crc32(const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    uint32_t c = 0xFFFFFFFFu, k;
    while (n--) {
        c ^= *b++;
        for (k = 0; k < 8u; k++)
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}

static void key(char *k, uint32_t obj, int copy)
{
    /* "o<obj><a|b>" */
    char d[11];
    int n = 0;
    do {
        d[n++] = (char)('0' + obj % 10u);
        obj /= 10u;
    } while (obj);
    *k++ = 'o';
    while (n)
        *k++ = d[--n];
    *k++ = copy ? 'b' : 'a';
    *k = 0;
}

/* read copy `copy` into scratch; 0 ok and valid (*h filled), -1 none, -2 bad */
static int read_copy(uint32_t obj, int copy, hdr_t *h)
{
    char k[16];
    uint32_t len = sizeof scratch;
    int rc;
    key(k, obj, copy);
    rc = be->get(k, scratch, &len);
    if (rc)
        return rc == -1 ? -1 : -2;
    if (len < sizeof(hdr_t))
        return -2;
    memcpy(h, scratch, sizeof *h);
    if (h->len > CP_STORE_MAX || h->len + sizeof(hdr_t) != len || cp_crc32(scratch + sizeof(hdr_t), h->len) != h->crc)
        return -2;
    return 0;
}

int cp_store_init_with(const char *ns, const cp_store_backend_t *backend)
{
    be = backend;
    return be->open ? be->open(ns) : 0;
}

/* which copies are valid, and which is newest: returns the newest copy (0/1), -1 none at all, -2 none valid */
static int newest(uint32_t obj, hdr_t h[2], int ok[2], int *any)
{
    int c;
    *any = 0;
    for (c = 0; c < 2; c++) {
        int rc = read_copy(obj, c, &h[c]);
        ok[c] = rc == 0;
        if (rc != -1)
            *any = 1;
    }
    if (ok[0] && ok[1])
        return h[1].gen > h[0].gen ? 1 : 0;
    if (ok[0] || ok[1])
        return ok[1] ? 1 : 0;
    return *any ? -2 : -1;
}

static int load_copy(uint32_t obj, int copy, void *dst, uint32_t max)
{
    hdr_t h;
    if (read_copy(obj, copy, &h))
        return -2;
    if (h.len > max)
        return -2;
    memcpy(dst, scratch + sizeof(hdr_t), h.len);
    return (int)h.len;
}

int cp_store_load(uint32_t obj, void *dst, uint32_t max)
{
    hdr_t h[2];
    int ok[2], any, n = newest(obj, h, ok, &any);
    return n < 0 ? n : load_copy(obj, n, dst, max);
}

int cp_store_load_prev(uint32_t obj, void *dst, uint32_t max)
{
    hdr_t h[2];
    int ok[2], any, n = newest(obj, h, ok, &any);
    if (n < 0 || !ok[1 - n])
        return -1;
    return load_copy(obj, 1 - n, dst, max);
}

int cp_store_save(uint32_t obj, const void *src, uint32_t len)
{
    hdr_t h[2], w;
    int ok[2], any, n, target;
    char k[16];
    if (len > CP_STORE_MAX)
        return -3;
    n = newest(obj, h, ok, &any);
    target = n < 0 ? 0 : 1 - n;                  /* the older (or a bad) copy */
    w.gen = n < 0 ? 1u : h[n].gen + 1u;
    w.len = len;
    w.crc = cp_crc32(src, len);
    memcpy(scratch, &w, sizeof w);
    memcpy(scratch + sizeof w, src, len);
    key(k, obj, target);
    return be->set(k, scratch, (uint32_t)sizeof w + len) ? -9 : 0;
}

int cp_store_erase_all(void) { return be->erase_all(); }

void cp_store_test_corrupt_newest(uint32_t obj)
{
    hdr_t h[2];
    int ok[2], any, n = newest(obj, h, ok, &any);
    char k[16];
    uint32_t len = sizeof scratch;
    if (n < 0)
        return;
    key(k, obj, n);
    if (be->get(k, scratch, &len) || len <= sizeof(hdr_t))
        return;
    scratch[sizeof(hdr_t)] ^= 0xFFu;
    be->set(k, scratch, len);
}
