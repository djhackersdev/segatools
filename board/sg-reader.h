#pragma once

#include <windows.h>

#include <stdbool.h>

struct aime_config {
    bool enable;
};

HRESULT sg_reader_hook_init(
        const struct aime_config *cfg,
        unsigned int port_no);
