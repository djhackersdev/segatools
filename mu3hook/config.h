#pragma once

#include <stddef.h>

#include "board/config.h"

#include "hooklib/gfx.h"

#include "platform/config.h"

struct mu3_hook_config {
    struct platform_config platform;
    struct aime_config aime;
    struct gfx_config gfx;
};

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename);
