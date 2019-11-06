#pragma once

#include <windows.h>

#include <stdbool.h>

struct zinput_config {
    bool enable;
};

HRESULT zinput_hook_init(struct zinput_config *cfg);
