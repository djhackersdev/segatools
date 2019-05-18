#pragma once

#include <stddef.h>

#include "amex/config.h"

#include "platform/config.h"

struct idz_hook_config {
    struct nu_config nu;
    struct amex_config amex;
};

void idz_hook_config_load(
        struct idz_hook_config *cfg,
        const wchar_t *filename);
