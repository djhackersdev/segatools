#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "hooklib/config.h"

void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"gfx", L"enable", 1, filename);
    cfg->windowed = GetPrivateProfileIntW(L"gfx", L"windowed", 0, filename);
    cfg->framed = GetPrivateProfileIntW(L"gfx", L"framed", 1, filename);
}
