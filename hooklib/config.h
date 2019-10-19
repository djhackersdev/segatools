#pragma once

#include <stdbool.h>
#include <stddef.h>

struct gfx_config {
    bool enable;
    bool windowed;
    bool framed;
};

void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename);
