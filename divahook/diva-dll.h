#pragma once

#include <windows.h>

#include "divaio/divaio.h"

struct diva_dll {
    uint16_t api_version;
    HRESULT (*jvs_init)(void);
    void (*jvs_poll)(uint8_t *opbtn, uint8_t *beams);
    void (*jvs_read_coin_counter)(uint16_t *total);
    HRESULT (*slider_init)(void);
    void (*slider_start)(diva_io_slider_callback_t callback);
    void (*slider_stop)(void);
    void (*slider_set_leds)(const uint8_t *rgb);
};

struct diva_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct diva_dll diva_dll;

HRESULT diva_dll_init(const struct diva_dll_config *cfg, HINSTANCE self);
