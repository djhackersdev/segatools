#pragma once

#include <windows.h>

#include "platform/config.h"

HRESULT nusec_hook_init(
        const struct nusec_config *cfg,
        const char *game_id,
        const char *platform_id);
