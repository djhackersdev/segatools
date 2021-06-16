#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "hooklib/config.h"
#include "hooklib/gfx.h"
#include "hooklib/dvd.h"

void gfx_config_load(struct gfx_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"gfx", L"enable", 1, filename);
    cfg->windowed = GetPrivateProfileIntW(L"gfx", L"windowed", 0, filename);
    cfg->framed = GetPrivateProfileIntW(L"gfx", L"framed", 1, filename);
    cfg->monitor = GetPrivateProfileIntW(L"gfx", L"monitor", 0, filename);
}

void dvd_config_load(struct dvd_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"dvd", L"enable", 1, filename);
}
