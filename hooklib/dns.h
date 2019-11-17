#pragma once

#include <windows.h>

#include <stddef.h>

// if to_src is NULL, all lookups for from_src will fail
HRESULT dns_hook_push(const wchar_t *from_src, const wchar_t *to_src);

