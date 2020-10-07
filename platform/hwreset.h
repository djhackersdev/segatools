#pragma once

#include <windows.h>

#include <stdbool.h>

struct hwreset_config {
    bool enable;
};

HRESULT hwreset_hook_init(const struct hwreset_config *cfg);
