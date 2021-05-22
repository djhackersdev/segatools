#pragma once

#include <stddef.h>
#include <stdint.h>

#include "hook/table.h"

HRESULT dll_hook_push(
        HMODULE redir_mod,
        const wchar_t *name,
        const struct hook_symbol *syms,
        size_t nsyms);
