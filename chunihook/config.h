#pragma once

#include <stddef.h>

#include "amex/config.h"

#include "hooklib/config.h"

#include "platform/config.h"

struct chuni_hook_config {
    struct nu_config nu;
    struct amex_config amex;
    struct gfx_config gfx;
};

void chuni_hook_config_load(
        struct chuni_hook_config *cfg,
        const wchar_t *filename);
