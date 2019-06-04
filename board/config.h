#pragma once

#include <stdbool.h>
#include <stddef.h>

struct aime_config {
    bool enable;
};

void aime_config_load(struct aime_config *cfg, const wchar_t *filename);
