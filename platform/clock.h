#pragma once

#include <windows.h>

#include <stdbool.h>

struct clock_config {
    bool timezone;
    bool timewarp;
    bool writeable;
};

HRESULT clock_hook_init(const struct clock_config *cfg);
