#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "hooklib/dvd.h"

#include "idzhook/idz-dll.h"
#include "idzhook/zinput.h"

#include "platform/platform.h"

struct idz_hook_config {
    struct platform_config platform;
    struct amex_config amex;
    struct aime_config aime;
    struct dvd_config dvd;
    struct idz_dll_config dll;
    struct zinput_config zinput;
};

void idz_dll_config_load(
        struct idz_dll_config *cfg,
        const wchar_t *filename);

void idz_hook_config_load(
        struct idz_hook_config *cfg,
        const wchar_t *filename);

void zinput_config_load(struct zinput_config *cfg, const wchar_t *filename);
