#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>

struct pcbid_config {
    bool enable;
    wchar_t serial_no[17];
};

HRESULT pcbid_hook_init(const struct pcbid_config *cfg);
