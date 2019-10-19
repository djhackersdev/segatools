#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "iccard/mifare.h"

HRESULT aime_card_populate(
        struct mifare *mifare,
        const uint8_t *luid,
        size_t nbytes);
