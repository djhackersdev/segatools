#pragma once

#include <windows.h>

struct misc_config {
    bool enable;
};

HRESULT misc_hook_init(const struct misc_config *cfg, const char *platform_id);
