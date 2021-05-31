#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/config.h"
#include "board/sg-reader.h"

#include "chunihook/config.h"

#include "hooklib/config.h"
#include "hooklib/gfx.h"

#include "platform/config.h"
#include "platform/platform.h"

void chuni_dll_config_load(
        struct chuni_dll_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"chuniio",
            L"path",
            L"",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void slider_config_load(struct slider_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"slider", L"enable", 1, filename);
}

void chuni_hook_config_load(
        struct chuni_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    memset(cfg, 0, sizeof(*cfg));

    platform_config_load(&cfg->platform, filename);
    amex_config_load(&cfg->amex, filename);
    aime_config_load(&cfg->aime, filename);
    gfx_config_load(&cfg->gfx, filename);
    chuni_dll_config_load(&cfg->dll, filename);
    slider_config_load(&cfg->slider, filename);
}
