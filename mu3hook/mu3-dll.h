#pragma once

#include <windows.h>

#include "mu3io/mu3io.h"

struct mu3_dll {
    uint16_t api_version;
    HRESULT (*init)(void);
    HRESULT (*poll)(void);
    void (*get_opbtns)(uint8_t *opbtn);
    void (*get_gamebtns)(uint8_t *left, uint8_t *right);
    void (*get_lever)(int16_t *pos);
};

struct mu3_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct mu3_dll mu3_dll;

HRESULT mu3_dll_init(const struct mu3_dll_config *cfg, HINSTANCE self);
