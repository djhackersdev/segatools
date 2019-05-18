#include <windows.h>

#include <string.h>
#include <stdlib.h>

#include "hook/table.h"

#include "hooklib/dll.h"
#include "hooklib/reg.h"

#include "platform/amvideo.h"

#include "util/dprintf.h"

/* Hook functions */

static int amDllVideoOpen(void *ctx);
static int amDllVideoClose(void *ctx);
static int amDllVideoSetResolution(void *ctx, void *param);
static int amDllVideoGetVBiosVersion(void *ctx, char *dest, size_t nchars);

static HRESULT amvideo_reg_read_name(void *bytes, uint32_t *nbytes);
static HRESULT amvideo_reg_read_port_X(void *bytes, uint32_t *nbytes);
static HRESULT amvideo_reg_read_resolution_1(void *bytes, uint32_t *nbytes);
static HRESULT amvideo_reg_read_use_segatiming(void *bytes, uint32_t *nbytes);

static const wchar_t amvideo_dll_name[] = L"$amvideo";

static const struct reg_hook_val amvideo_reg_vals[] = {
    {
        .name       = L"name",
        .read       = amvideo_reg_read_name,
        .type       = REG_SZ,
    }
};

static const struct reg_hook_val amvideo_reg_mode_vals[] = {
    {
        .name       = L"port_1",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_2",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_3",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_4",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_5",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_6",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_7",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"port_8",
        .read       = amvideo_reg_read_port_X,
        .type       = REG_DWORD
    }, {
        .name       = L"resolution_1",
        .read       = amvideo_reg_read_resolution_1,
        .type       = REG_SZ,
    }, {
        .name       = L"use_segatiming",
        .read       = amvideo_reg_read_use_segatiming,
        .type       = REG_DWORD,
    }
};

static const struct hook_symbol amvideo_syms[] = {
    {
        .ordinal    = 1,
        .name       = "amDllVideoOpen",
        .patch      = amDllVideoOpen,
    }, {
        .ordinal    = 2,
        .name       = "amDllVideoClose",
        .patch      = amDllVideoClose,
    }, {
        .ordinal    = 3,
        .name       = "amDllVideoSetResolution",
        .patch      = amDllVideoSetResolution,
    }, {
        .ordinal    = 4,
        .name       = "amDllVideoGetVBiosVersion",
        .patch      = amDllVideoGetVBiosVersion,
    }
};

HRESULT amvideo_hook_init(const struct amvideo_config *cfg, HMODULE redir_mod)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty\\amVideo",
            amvideo_reg_vals,
            _countof(amvideo_reg_vals));

    if (FAILED(hr)) {
        return hr;
    }

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty\\sgsetdisplaysetting\\CurrentSetting",
            amvideo_reg_mode_vals,
            _countof(amvideo_reg_mode_vals));

    if (FAILED(hr)) {
        return hr;
    }

    hr = dll_hook_push(
            redir_mod,
            amvideo_dll_name,
            amvideo_syms,
            _countof(amvideo_syms));

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static int amDllVideoOpen(void *ctx)
{
    dprintf("AmVideo: %s(%p)\n", __func__, ctx);

    return 0;
}

static int amDllVideoClose(void *ctx)
{
    dprintf("AmVideo: %s(%p)\n", __func__, ctx);

    return 0;
}

static int amDllVideoSetResolution(void *ctx, void *param)
{
    dprintf("AmVideo: %s(%p, %p)\n", __func__, ctx, param);

    return 0;
}

static int amDllVideoGetVBiosVersion(void *ctx, char *dest, size_t nchars)
{
    dprintf("AmVideo: %s(%p, %p, %i)\n", __func__, ctx, dest, (int) nchars);
    strcpy(dest, "01.02.03.04.05");

    return 0;
}

static HRESULT amvideo_reg_read_name(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, amvideo_dll_name);
}

static HRESULT amvideo_reg_read_port_X(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, 1);
}

static HRESULT amvideo_reg_read_resolution_1(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, L"1920x1080");
}

static HRESULT amvideo_reg_read_use_segatiming(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, 0);
}
