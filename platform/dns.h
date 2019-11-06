#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>

struct dns_config {
    bool enable;
    wchar_t router[128];
    wchar_t startup[128];
    wchar_t billing[128];
    wchar_t aimedb[128];
};

HRESULT dns_platform_hook_init(const struct dns_config *cfg);
