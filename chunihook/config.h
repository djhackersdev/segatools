#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "amex/config.h"

#include "hooklib/config.h"

#include "platform/config.h"

struct slider_config {
    bool enable;
};

struct chuni_hook_config {
    struct nu_config nu;
    struct amex_config amex;
    struct gfx_config gfx;
    struct slider_config slider;
};

void slider_config_load(struct slider_config *cfg, const wchar_t *filename);
void chuni_hook_config_load(
        struct chuni_hook_config *cfg,
        const wchar_t *filename);
