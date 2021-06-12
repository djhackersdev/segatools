#pragma once

#include <windows.h>

#include "chuniio/chuniio.h"

struct chuni_dll {
    uint16_t api_version;
    HRESULT (*jvs_init)(void);
    void (*jvs_poll)(uint8_t *opbtn, uint8_t *beams);
    void (*jvs_read_coin_counter)(uint16_t *total);
    HRESULT (*slider_init)(void);
    void (*slider_start)(chuni_io_slider_callback_t callback);
    void (*slider_stop)(void);
    void (*slider_set_leds)(const uint8_t *rgb);
};

struct chuni_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct chuni_dll chuni_dll;

HRESULT chuni_dll_init(const struct chuni_dll_config *cfg, HINSTANCE self);
