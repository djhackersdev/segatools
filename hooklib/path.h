#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>

typedef HRESULT (*path_hook_t)(
        const wchar_t *src,
        wchar_t *dest,
        size_t *count);

HRESULT path_hook_push(path_hook_t hook);
int path_compare_w(const wchar_t *string1, const wchar_t *string2, size_t count);

static inline bool path_is_separator_w(wchar_t c)
{
    return c == L'\\' || c == L'/';
}
