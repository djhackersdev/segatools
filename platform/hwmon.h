#pragma once

#include <windows.h>

#include "platform/config.h"

HRESULT hwmon_hook_init(const struct hwmon_config *cfg);
