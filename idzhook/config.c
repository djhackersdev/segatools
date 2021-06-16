#include <assert.h>
#include <stddef.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/config.h"
#include "board/sg-reader.h"

#include "hooklib/config.h"
#include "hooklib/dvd.h"

#include "idzhook/config.h"
#include "idzhook/idz-dll.h"

#include "platform/config.h"
#include "platform/platform.h"

void idz_dll_config_load(
        struct idz_dll_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"idzio",
            L"path",
            L"",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void idz_hook_config_load(
        struct idz_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    amex_config_load(&cfg->amex, filename);
    aime_config_load(&cfg->aime, filename);
    idz_dll_config_load(&cfg->dll, filename);
    zinput_config_load(&cfg->zinput, filename);
    dvd_config_load(&cfg->dvd, filename);
}

void zinput_config_load(struct zinput_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"zinput", L"enable", 1, filename);
}
