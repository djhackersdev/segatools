#pragma once

#include <windows.h>

#include <stddef.h>

HRESULT dns_hook_push(const wchar_t *from_src, const wchar_t *to_src);

