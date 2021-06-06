#pragma once

#include <stddef.h>

#include "board/config.h"

#include "hooklib/gfx.h"

#include "mu3hook/mu3-dll.h"

#include "platform/config.h"

struct mu3_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct gfx_config gfx;
    struct mu3_dll_config dll;
};

void mu3_dll_config_load(
        struct mu3_dll_config *cfg,
        const wchar_t *filename);

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename);
