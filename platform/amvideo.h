#pragma once

#include <windows.h>

#include <stdbool.h>

struct amvideo_config {
    bool enable;
};

HRESULT amvideo_hook_init(const struct amvideo_config *cfg, HMODULE redir_mod);
