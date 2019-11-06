#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "board/sg-reader.h"

void aime_config_load(struct aime_config *cfg, const wchar_t *filename);
