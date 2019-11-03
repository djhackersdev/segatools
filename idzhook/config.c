#include <assert.h>
#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "idzhook/config.h"

#include "platform/config.h"

void idz_hook_config_load(
        struct idz_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    amex_config_load(&cfg->amex, filename);
    aime_config_load(&cfg->aime, filename);
    zinput_config_load(&cfg->zinput, filename);
}

void zinput_config_load(struct zinput_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"zinput", L"enable", 1, filename);
}
