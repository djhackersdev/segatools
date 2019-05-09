#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>

enum {
    FDSHARK_FORCE_SYNC = 0x1,
    FDSHARK_TRACE_READ = 0x2,
    FDSHARK_TRACE_WRITE = 0x4,
    FDSHARK_TRACE_IOCTL = 0x8,
    FDSHARK_ALL_FLAGS_ = 0xF,
};

HRESULT fdshark_hook_init(const wchar_t *filename, int flags);
