#pragma once

#include <windows.h>

#include <stdbool.h>

struct slider_config {
    bool enable;
};

HRESULT slider_hook_init(const struct slider_config *cfg);
