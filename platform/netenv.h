#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

#include "platform/config.h"

HRESULT netenv_hook_init(
        const struct netenv_config *cfg,
        const struct nusec_config *kc_cfg);

