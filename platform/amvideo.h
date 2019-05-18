#pragma once

#include <windows.h>

#include "platform/config.h"

HRESULT amvideo_hook_init(const struct amvideo_config *cfg, HMODULE redir_mod);
