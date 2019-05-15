#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

struct reg_hook_val {
    const wchar_t *name;
    HRESULT (*read)(void *bytes, uint32_t *nbytes);
    HRESULT (*write)(const void *bytes, uint32_t nbytes);
    uint32_t type;
};

HRESULT reg_hook_push_key(
        HKEY root,
        const wchar_t *name,
        const struct reg_hook_val *vals,
        size_t nvals);

HRESULT reg_hook_read_bin(
        void *bytes,
        uint32_t *nbytes,
        const void *src_bytes,
        size_t src_nbytes);

HRESULT reg_hook_read_u32(
        void *bytes,
        uint32_t *nbytes,
        uint32_t src);

HRESULT reg_hook_read_wstr(
        void *bytes,
        uint32_t *nbytes,
        const wchar_t *src);
