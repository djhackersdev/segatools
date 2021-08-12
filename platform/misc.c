#include <windows.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hook/table.h"

#include "hooklib/reg.h"

#include "platform/misc.h"

#include "util/dprintf.h"

static BOOL WINAPI misc_ExitWindowsEx(unsigned int flags, uint32_t reason);

static HRESULT misc_read_os_version(void *bytes, uint32_t *nbytes);
static HRESULT misc_read_app_loader_count(void *bytes, uint32_t *nbytes);
static HRESULT misc_read_cpu_temp_error(void *bytes, uint32_t *nbytes);
static HRESULT misc_read_cpu_temp_warning(void *bytes, uint32_t *nbytes);
static HRESULT misc_read_platform_id(void *bytes, uint32_t *nbytes);

static const struct hook_symbol misc_syms[] = {
    {
        .name   = "ExitWindowsEx",
        .patch  = misc_ExitWindowsEx,
    }
};

static const struct reg_hook_val misc_root_keys[] = {
    {
        .name   = L"OSVersion",
        .read   = misc_read_os_version,
        .type   = REG_SZ,
    }
};

static const struct reg_hook_val misc_master_keys[] = {
    {
        .name   = L"AppLoaderCount",
        .read   = misc_read_app_loader_count,
        .type   = REG_DWORD,
    }, {
        /* Black-hole val, list it here so we don't get a warning msg */
        .name   = L"NextProcess",
        .type   = REG_SZ,
    }, {
        /* ditto */
        .name   = L"SystemError",
        .type   = REG_SZ,
    }
};

static const struct reg_hook_val misc_static_keys[] = {
    {
        .name   = L"CpuTempError",
        .read   = misc_read_cpu_temp_error,
        .type   = REG_DWORD,
    }, {
        .name   = L"CpuTempWarning",
        .read   = misc_read_cpu_temp_warning,
        .type   = REG_DWORD,
    }, {
        .name   = L"PlatformId",
        .read   = misc_read_platform_id,
        .type   = REG_SZ,
    }
};

static wchar_t misc_platform_id[5];

HRESULT misc_hook_init(const struct misc_config *cfg, const char *platform_id)
{
    HRESULT hr;

    assert(cfg != NULL);
    assert(platform_id != NULL && strlen(platform_id) == 4);

    if (!cfg->enable) {
        return S_FALSE;
    }

    /* Set platform ID registry value */

    mbstowcs_s(
            NULL,
            misc_platform_id,
            _countof(misc_platform_id),
            platform_id,
            _countof(misc_platform_id) - 1);

    /* Add hardcoded dummy keys */

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty",
            misc_root_keys,
            _countof(misc_root_keys));

    if (FAILED(hr)) {
        return hr;
    }

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty\\static",
            misc_static_keys,
            _countof(misc_static_keys));

    if (FAILED(hr)) {
        return hr;
    }

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty\\Master",
            misc_master_keys,
            _countof(misc_master_keys));

    if (FAILED(hr)) {
        return hr;
    }

    /* Apply function hooks */

    hook_table_apply(NULL, "user32.dll", misc_syms, _countof(misc_syms));

    return S_OK;
}

static BOOL WINAPI misc_ExitWindowsEx(unsigned int flags, uint32_t reason)
{
    dprintf("Misc: Blocked system reboot\n");

    return TRUE;
}

static HRESULT misc_read_os_version(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, L"0_0_0");
}

static HRESULT misc_read_app_loader_count(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, 1);
}

static HRESULT misc_read_cpu_temp_error(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, 100);
}

static HRESULT misc_read_cpu_temp_warning(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, 95);
}

static HRESULT misc_read_platform_id(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, misc_platform_id);
}
