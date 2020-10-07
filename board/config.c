#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "board/aime-dll.h"
#include "board/config.h"
#include "board/sg-reader.h"

static void aime_dll_config_load(struct aime_dll_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"aimeio",
            L"path",
            L"",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void aime_config_load(struct aime_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    aime_dll_config_load(&cfg->dll, filename);
    cfg->enable = GetPrivateProfileIntW(L"aime", L"enable", 1, filename);
}

void io4_config_load(struct io4_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"io4", L"enable", 1, filename);
}
