#pragma once

#include <stddef.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "platform/platform.h"

struct diva_hook_config {
    struct platform_config platform;
    struct amex_config amex;
    struct aime_config aime;
};

void diva_hook_config_load(
        struct diva_hook_config *cfg,
        const wchar_t *filename);
