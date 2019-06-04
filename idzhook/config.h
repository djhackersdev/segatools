#pragma once

#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "platform/config.h"

struct idz_hook_config {
    struct nu_config nu;
    struct amex_config amex;
    struct aime_config aime;
};

void idz_hook_config_load(
        struct idz_hook_config *cfg,
        const wchar_t *filename);
