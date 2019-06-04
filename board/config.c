#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "board/config.h"

void aime_config_load(struct aime_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"aime", L"enable", 1, filename);
}
