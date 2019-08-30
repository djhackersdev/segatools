#pragma once

#include <windows.h>

#include "platform/config.h"

HRESULT platform_hook_init_alls(
        const struct alls_config *cfg,
        const char *game_id,
        const char *platform_id,
        HMODULE redir_mod);

HRESULT platform_hook_init_nu(
        const struct nu_config *cfg,
        const char *game_id,
        const char *platform_id,
        HMODULE redir_mod);
