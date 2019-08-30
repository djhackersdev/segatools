#pragma once

#include <stddef.h>

#include "board/config.h"

#include "platform/config.h"

struct mu3_hook_config {
    struct alls_config alls;
    struct aime_config aime;
};

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename);
