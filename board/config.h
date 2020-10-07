#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "board/io4.h"
#include "board/sg-reader.h"

void aime_config_load(struct aime_config *cfg, const wchar_t *filename);
void io4_config_load(struct io4_config *cfg, const wchar_t *filename);
