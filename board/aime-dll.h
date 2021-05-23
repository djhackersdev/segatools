#pragma once

#include <windows.h>

#include "aimeio/aimeio.h"

struct aime_dll {
    uint16_t api_version;
    HRESULT (*init)(void);
    HRESULT (*nfc_poll)(uint8_t unit_no);
    HRESULT (*nfc_get_aime_id)(
            uint8_t unit_no,
            uint8_t *luid,
            size_t luid_size);
    HRESULT (*nfc_get_felica_id)(uint8_t unit_no, uint64_t *IDm);
    void (*led_set_color)(uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b);
};

struct aime_dll_config {
    wchar_t path[MAX_PATH];
};

extern struct aime_dll aime_dll;

HRESULT aime_dll_init(const struct aime_dll_config *cfg, HINSTANCE self);
