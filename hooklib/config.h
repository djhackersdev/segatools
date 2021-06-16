#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "hooklib/gfx.h"
#include "hooklib/dvd.h"

void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename);
void dvd_config_load(struct dvd_config *cfg, const wchar_t *filename);
