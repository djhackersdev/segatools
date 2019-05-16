#pragma once

#include <windows.h>

#include <stddef.h>

typedef HRESULT (*path_hook_t)(
        const wchar_t *src,
        wchar_t *dest,
        size_t *count);

HRESULT path_hook_push(path_hook_t hook);
