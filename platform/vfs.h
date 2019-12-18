#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>

struct vfs_config {
    bool enable;
    wchar_t amfs[MAX_PATH];
    wchar_t appdata[MAX_PATH];
    wchar_t option[MAX_PATH];
};

HRESULT vfs_hook_init(const struct vfs_config *config);
