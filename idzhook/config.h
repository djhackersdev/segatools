#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "platform/config.h"

struct zinput_config {
    bool enable;
};

struct idz_hook_config {
    struct platform_config platform;
    struct amex_config amex;
    struct aime_config aime;
    struct zinput_config zinput;
};

void idz_hook_config_load(
        struct idz_hook_config *cfg,
        const wchar_t *filename);

void zinput_config_load(struct zinput_config *cfg, const wchar_t *filename);
