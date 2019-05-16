#pragma once

#include <windows.h>

#include "amex/config.h"

DEFINE_GUID(
        sram_guid,
        0x741B5FCA,
        0x4635,
        0x4443,
        0xA7, 0xA0, 0x57, 0xCA, 0x7B, 0x50, 0x6A, 0x49);

HRESULT sram_hook_init(const struct sram_config *cfg);
