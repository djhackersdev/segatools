#pragma once

#include <windows.h>

#include "board/config.h"

HRESULT sg_reader_hook_init(
        const struct aime_config *cfg,
        unsigned int port_no);
