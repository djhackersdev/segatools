#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/config.h"
#include "board/sg-reader.h"

#include "divahook/config.h"

#include "platform/config.h"
#include "platform/platform.h"

void diva_dll_config_load(
        struct diva_dll_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"divaio",
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

void diva_hook_config_load(
        struct diva_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    amex_config_load(&cfg->amex, filename);
    aime_config_load(&cfg->aime, filename);
    diva_dll_config_load(&cfg->dll, filename);
    slider_config_load(&cfg->slider, filename);
}
