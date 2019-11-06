#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ds_config {
    bool enable;
    uint8_t region;
    wchar_t serial_no[17];
};

DEFINE_GUID(
        ds_guid,
        0x279A9F67,
        0x348F,
        0x41C9,
        0xA4, 0xC4, 0xDF, 0xDB, 0x8A, 0xE8, 0xE5, 0xE0);

HRESULT ds_hook_init(const struct ds_config *cfg);
