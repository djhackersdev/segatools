#pragma once

#include <windows.h>

#include "platform/config.h"

HRESULT platform_hook_init(
        const struct platform_config *cfg,
        const char *game_id,
        const char *platform_id,
        HMODULE redir_mod);
