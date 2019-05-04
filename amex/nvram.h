#pragma once

#include <windows.h>

#include <stddef.h>

HRESULT nvram_open_file(HANDLE *out, const wchar_t *path, size_t size);
