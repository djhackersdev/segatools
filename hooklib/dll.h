#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hook/table.h"

struct dll_symbol {
    void *ptr;
    const char *name;
    uint16_t ordinal;
};

HRESULT dll_hook_push(
        HMODULE redir_mod,
        const wchar_t *name,
        const struct hook_symbol *syms,
        size_t nsyms);
