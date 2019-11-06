#pragma once

#include <windows.h>

#include <stdbool.h>

struct hwmon_config {
    bool enable;
};

HRESULT hwmon_hook_init(const struct hwmon_config *cfg);
