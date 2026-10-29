/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M. Geisler (genkigenki) */
#include "cp_overlay.h"
#include "cp_display.h"

static const uint8_t BLACK_KEYS[CP_OV_NKEYS] = {   /* F3 F#3 G3 ... G5: 1 = black */
    0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0};

static struct {
    char letter[8];
    uint32_t n_enc, keys, dim, btn;
    int play_red, knob, drawn;     /* drawn: the state below was drawn */
    uint32_t d_keys, d_dim, d_btn;
    int d_play, d_knob, d_strategy;
} ov;

/* 5 x 7 glyphs, a byte per row (bit 4 = left column) */
static const struct { char c; uint8_t row[7]; } FONT[] = {
    {'0', {14, 17, 19, 21, 25, 17, 14}}, {'1', {4, 12, 4, 4, 4, 4, 14}},   {'2', {14, 17, 1, 2, 4, 8, 31}},
    {'3', {31, 2, 4, 2, 1, 17, 14}},    {'4', {2, 6, 10, 18, 31, 2, 2}},   {'5', {31, 16, 30, 1, 1, 17, 14}},
    {'6', {6, 8, 16, 30, 17, 17, 14}},  {'7', {31, 1, 2, 4, 8, 8, 8}},     {'8', {14, 17, 17, 14, 17, 17, 14}},
    {'9', {14, 17, 17, 15, 1, 2, 12}},  {'A', {14, 17, 17, 31, 17, 17, 17}}, {'P', {30, 17, 17, 30, 16, 16, 16}},
    {'S', {15, 16, 16, 14, 1, 1, 30}},  {'K', {17, 18, 20, 24, 20, 18, 17}}, {'M', {17, 27, 21, 21, 17, 17, 17}},
};

void cp_overlay_init(const char *const *names, uint32_t n)
{
    uint32_t i;
    ov.n_enc = n > 8u ? 8u : n;
    for (i = 0; i < ov.n_enc; i++) {
        const char *s = names[i], *e = s;
        while (*e)
            e++;
        ov.letter[i] = e > s && e[-1] >= '0' && e[-1] <= '9' ? e[-1] : s[0];
    }
    ov.knob = -1;
    ov.drawn = 0;
}

void cp_overlay_leds(uint32_t keys_lit, uint32_t keys_dim, uint32_t buttons_lit, int play_red)
{
    ov.keys = keys_lit, ov.dim = keys_dim, ov.btn = buttons_lit, ov.play_red = play_red != 0;
}

void cp_overlay_knob(int active) { ov.knob = active >= 0 && (uint32_t)active < ov.n_enc ? active : -1; }

static void rect(uint16_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    for (j = y; j < y + h && j < CP_LCD_H; j++)
        for (i = x; i < x + w && i < CP_LCD_W; i++)
            fb[j * CP_LCD_W + i] = c;
}

static uint16_t key_colour(uint32_t k)
{
    return (ov.keys >> k) & 1u ? CP_OV_LIT : (ov.dim >> k) & 1u ? CP_OV_DIM : 0u;
}

static uint16_t btn_colour(uint32_t b)
{
    if (!((ov.btn >> b) & 1u))
        return 0u;
    return b == CP_OV_PLAY && ov.play_red ? CP_OV_REC : CP_OV_BTN;
}

static void glyph(uint16_t *fb, uint32_t x, uint32_t y, char c)
{
    uint32_t i, r, b;
    for (i = 0; i < sizeof FONT / sizeof FONT[0]; i++)
        if (FONT[i].c == c)
            for (r = 0; r < 7u; r++)
                for (b = 0; b < 5u; b++)
                    if ((FONT[i].row[r] >> (4u - b)) & 1u)
                        fb[(y + r) * CP_LCD_W + x + b] = CP_OV_LIT;
}

void cp_overlay_draw(void)
{
    uint16_t *fb = cp_fb();
    uint32_t k, top = cp_fb_ui_h();
    int s = cp_display_strategy();
    if (!fb)
        return;
    if (ov.drawn && ov.d_keys == ov.keys && ov.d_dim == ov.dim && ov.d_btn == ov.btn && ov.d_play == ov.play_red &&
        ov.d_knob == ov.knob && ov.d_strategy == s)
        return;
    if (s == CP_SCREEN_SHRINK) {                    /* the side panels: 52 px left and right of the square */
        rect(fb, 0, 0, 52, CP_LCD_H, 0);
        rect(fb, 188, 0, 52, CP_LCD_H, 0);
        for (k = 0; k < CP_OV_NKEYS; k++)
            rect(fb, BLACK_KEYS[k] ? 22u : 0u, 5u * k, BLACK_KEYS[k] ? 30u : 51u, 4, key_colour(k));
        for (k = 0; k < CP_OV_NBTN; k++)
            rect(fb, 190, 9u * k, 48, 7, btn_colour(k));
        if (ov.knob >= 0)
            glyph(fb, 212, 127, ov.letter[ov.knob]);
        cp_display_mark_dirty(0, CP_LCD_H - 1u);
    } else {
        if (top >= CP_LCD_H)
            return;
        rect(fb, 0, top, CP_LCD_W, CP_LCD_H - top, 0);
        for (k = 0; k < CP_OV_NKEYS; k++)            /* rows top+1 .. 134 */
            rect(fb, 7u * k, top + 1u, BLACK_KEYS[k] ? 4u : 6u, CP_LCD_H - top - 1u, key_colour(k));
        for (k = 0; k < CP_OV_NBTN; k++)
            rect(fb, 190u + 3u * k, top + 1u, 2, CP_LCD_H - top - 1u, btn_colour(k));
        if (ov.knob >= 0 && CP_LCD_H - top >= 7u)
            glyph(fb, 234, CP_LCD_H - 7u, ov.letter[ov.knob]);
        cp_display_mark_dirty(top, CP_LCD_H - 1u);
    }
    ov.drawn = 1;
    ov.d_keys = ov.keys, ov.d_dim = ov.dim, ov.d_btn = ov.btn, ov.d_play = ov.play_red, ov.d_knob = ov.knob;
    ov.d_strategy = s;
}

uint16_t cp_overlay_pixel(uint32_t x, uint32_t y)
{
    uint16_t *fb = cp_fb();
    return fb && x < CP_LCD_W && y < CP_LCD_H ? fb[y * CP_LCD_W + x] : 0u;
}
