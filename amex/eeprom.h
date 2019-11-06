#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>

struct eeprom_config {
    bool enable;
    wchar_t path[MAX_PATH];
};

DEFINE_GUID(
        eeprom_guid,
        0xB7970F0C,
        0x31C4,
        0x45FF,
        0x96, 0x18, 0x0A, 0x24, 0x00, 0x94, 0xB2, 0x71);

HRESULT eeprom_hook_init(const struct eeprom_config *cfg);
