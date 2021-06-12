#pragma once

#include <windows.h>

#include "idzio/idzio.h"

struct idz_dll {
    uint16_t api_version;
    HRESULT (*jvs_init)(void);
    void (*jvs_read_analogs)(struct idz_io_analog_state *out);
    void (*jvs_read_buttons)(uint8_t *opbtn, uint8_t *gamebtn);
    void (*jvs_read_shifter)(uint8_t *gear);
    void (*jvs_read_coin_counter)(uint16_t *total);
};

struct idz_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct idz_dll idz_dll;

HRESULT idz_dll_init(const struct idz_dll_config *cfg, HINSTANCE self);
