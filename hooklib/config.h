#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "hooklib/gfx.h"

void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename);
