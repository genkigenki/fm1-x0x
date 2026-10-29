// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 M. Geisler (genkigenki)
// cp_store's blobs in NVS: the Launcher's nvs partition, one namespace per port (apps never see each other's).
#include <nvs.h>
#include <nvs_flash.h>
#include "cp_store.h"

static nvs_handle_t h;
static int opened;

static int nvs_open_ns(const char *ns)
{
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND)
        return -9;                                  // never erase the Launcher's NVS: report it instead
    if (opened)
        nvs_close(h);
    opened = nvs_open(ns, NVS_READWRITE, &h) == ESP_OK;
    return opened ? 0 : -9;
}

static int nvs_get(const char *key, void *dst, uint32_t *len)
{
    size_t n = *len;
    esp_err_t e;
    if (!opened)
        return -9;
    e = nvs_get_blob(h, key, dst, &n);
    if (e == ESP_ERR_NVS_NOT_FOUND)
        return -1;
    if (e == ESP_ERR_NVS_INVALID_LENGTH)
        return -2;
    if (e != ESP_OK)
        return -9;
    *len = (uint32_t)n;
    return 0;
}

static int nvs_set(const char *key, const void *src, uint32_t len)
{
    if (!opened || nvs_set_blob(h, key, src, len) != ESP_OK)
        return -9;
    return nvs_commit(h) == ESP_OK ? 0 : -9;
}

static int nvs_erase(void)
{
    if (!opened || nvs_erase_all(h) != ESP_OK)
        return -9;
    return nvs_commit(h) == ESP_OK ? 0 : -9;
}

extern "C" const cp_store_backend_t CP_STORE_NVS = {nvs_get, nvs_set, nvs_erase, nvs_open_ns};

extern "C" int cp_store_init(const char *ns) { return cp_store_init_with(ns, &CP_STORE_NVS); }
