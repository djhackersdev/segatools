#pragma once

#include <windows.h>

#include "amex/config.h"
#include "amex/jvs.h"

HRESULT amex_hook_init(
        const struct amex_config *cfg,
        jvs_provider_t jvs);
