#include <windows.h>
#include <shlwapi.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hooklib/path.h"
#include "hooklib/reg.h"

#include "platform/vfs.h"

#include "util/dprintf.h"

static void vfs_fixup_path(wchar_t *path, size_t max_count);
static HRESULT vfs_mkdir_rec(const wchar_t *path);
static HRESULT vfs_path_hook(const wchar_t *src, wchar_t *dest, size_t *count);
static HRESULT vfs_path_hook_nthome(
        const wchar_t *src,
        wchar_t *dest,
        size_t *count);
static HRESULT vfs_path_hook_option(
        const wchar_t *src,
        wchar_t *dest,
        size_t *count);
static HRESULT vfs_reg_read_amfs(void *bytes, uint32_t *nbytes);
static HRESULT vfs_reg_read_appdata(void *bytes, uint32_t *nbytes);

static wchar_t vfs_nthome_real[MAX_PATH];
static const wchar_t vfs_nthome[] = L"C:\\Documents and Settings\\AppUser";
static const size_t vfs_nthome_len = _countof(vfs_nthome) - 1;

static const wchar_t vfs_option[] = L"C:\\Mount\\Option";
static const size_t vfs_option_len = _countof(vfs_option) - 1;

static const struct reg_hook_val vfs_reg_vals[] = {
    {
        .name   = L"AMFS",
        .read   = vfs_reg_read_amfs,
        .type   = REG_SZ,
    }, {
        .name   = L"APPDATA",
        .read   = vfs_reg_read_appdata,
        .type   = REG_SZ
    },
};

static struct vfs_config vfs_config;

HRESULT vfs_hook_init(const struct vfs_config *config)
{
    wchar_t temp[MAX_PATH];
    size_t nthome_len;
    DWORD home_ok;
    HRESULT hr;

    assert(config != NULL);

    if (!config->enable) {
        return S_FALSE;
    }

    if (config->amfs[0] == L'\0') {
        dprintf("Vfs: FATAL: AMFS path not specified in INI file\n");

        return E_FAIL;
    }

    if (config->appdata[0] == L'\0') {
        dprintf("Vfs: FATAL: APPDATA path not specified in INI file\n");

        return E_FAIL;
    }

    if (config->option[0] == L'\0') {
        dprintf("Vfs: WARNING: OPTION path not specified in INI file\n");
    }

    home_ok = GetEnvironmentVariableW(
            L"USERPROFILE",
            vfs_nthome_real,
            _countof(vfs_nthome_real));

    if (!home_ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Vfs: Failed to query %%USERPROFILE%% env var: %x\n",
                (int) hr);

        return hr;
    }

    memcpy(&vfs_config, config, sizeof(*config));

    vfs_fixup_path(vfs_nthome_real, _countof(vfs_nthome_real));
    vfs_fixup_path(vfs_config.amfs, _countof(vfs_config.amfs));
    vfs_fixup_path(vfs_config.appdata, _countof(vfs_config.appdata));

    if (vfs_config.option[0] != L'\0') {
        vfs_fixup_path(vfs_config.option, _countof(vfs_config.option));
    }

    hr = vfs_mkdir_rec(vfs_config.amfs);

    if (FAILED(hr)) {
        dprintf("Vfs: Failed to create AMFS dir %S: %x\n",
                config->amfs,
                (int) hr);
    }

    hr = vfs_mkdir_rec(vfs_config.appdata);

    if (FAILED(hr)) {
        dprintf("Vfs: Failed to create APPDATA dir %S: %x\n",
                config->appdata,
                (int) hr);

        dprintf("Vfs: NOTE: SEGA Y: drive APPDATA, not Windows %%APPDATA%%.\n");
    }

    /* Need to create the temp subdirectory, not just nthome itself */

    nthome_len = wcslen(vfs_nthome_real);
    wcscpy_s(temp, _countof(temp), vfs_nthome_real);
    wcscpy_s(temp + nthome_len, _countof(temp) - nthome_len, L"temp");

    hr = vfs_mkdir_rec(temp);

    if (FAILED(hr)) {
        dprintf("Vfs: Failed to create %S: %x\n", temp, (int) hr);
    }

    /* Not auto-creating option directory as it is normally a read-only mount */

    hr = path_hook_push(vfs_path_hook);

    if (FAILED(hr)) {
        return hr;
    }

    hr = path_hook_push(vfs_path_hook_nthome);

    if (FAILED(hr)) {
        return hr;
    }

    if (vfs_config.option[0] != L'\0') {
        hr = path_hook_push(vfs_path_hook_option);

        if (FAILED(hr)) {
            return hr;
        }
    }

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty\\mount",
            vfs_reg_vals,
            _countof(vfs_reg_vals));

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static void vfs_fixup_path(wchar_t *path, size_t max_count)
{
    size_t count;
    wchar_t abspath[MAX_PATH];

    assert(path != NULL);
    /* Requirement for PathIsRelativeW */
    assert(max_count <= MAX_PATH);

    if (PathIsRelativeW(path)) {
        count = GetFullPathNameW(path, _countof(abspath), abspath, NULL);

        /* GetFullPathName's length return value is tricky, because it includes
           the NUL terminator on failure, but doesn't on success.
           Check if it fits the temp buf (else it's a failure and includes NUL
           anyway), then if it fits the target buf, NUL included. */
        if (count == 0 || count > _countof(abspath) || count >= max_count) {
            goto fail;
        }

        wcscpy_s(path, max_count, abspath);
    } else {
        count = wcslen(path);
    }

    if (path_is_separator_w(path[count - 1])) {
        return;
    }

    if (count + 2 > max_count) {
        goto fail;
    }

    path[count + 0] = L'\\';
    path[count + 1] = L'\0';
    return;

fail:
    dprintf("Vfs: FATAL: Path too long: %S\n", path);
    abort();
}

static HRESULT vfs_mkdir_rec(const wchar_t *path)
{
    wchar_t *copy;
    wchar_t *pos;
    wchar_t wc;
    HRESULT hr;
    DWORD attr;
    BOOL ok;

    assert(path != NULL);

    copy = _wcsdup(path);

    if (copy == NULL) {
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

        goto end;
    }

    pos = copy;

    do {
        wc = *pos;

        if (wc == L'\0' || wc == L'/' || wc == L'\\') {
            *pos = L'\0';
            attr = GetFileAttributesW(copy);

            if (attr == INVALID_FILE_ATTRIBUTES) {
                ok = CreateDirectoryW(copy, NULL);

                if (!ok) {
                    hr = HRESULT_FROM_WIN32(GetLastError());

                    goto end;
                }
            }

            *pos = wc;
        }

        pos++;
    } while (wc != L'\0');

    hr = S_OK;

end:
    free(copy);

    return hr;
}

static HRESULT vfs_path_hook(const wchar_t *src, wchar_t *dest, size_t *count)
{
    const wchar_t *redir;
    size_t required;
    size_t redir_len;

    assert(src != NULL);
    assert(count != NULL);

    if (src[0] == L'\0' || src[1] != L':' || !path_is_separator_w(src[2])) {
        return S_FALSE;
    }

    switch (src[0]) {
    case L'e':
    case L'E':
        redir = vfs_config.amfs;

        break;

    case L'y':
    case L'Y':
        redir = vfs_config.appdata;

        break;

    default:
        return S_FALSE;
    }

    /* Cut off <prefix>\, replace with redir path, count NUL terminator */

    redir_len = wcslen(redir);
    required = wcslen(src) - 3 + redir_len + 1;

    if (dest != NULL) {
        if (required > *count) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        wcscpy_s(dest, *count, redir);
        wcscpy_s(dest + redir_len, *count - redir_len, src + 3);
    }

    *count = required;

    return S_OK;
}

static HRESULT vfs_path_hook_nthome(
        const wchar_t *src,
        wchar_t *dest,
        size_t *count)
{
    size_t required;
    size_t redir_len;
    size_t shift;

    assert(src != NULL);
    assert(count != NULL);

    /* Case-insensitive check to see if src starts with vfs_nthome */

    if (path_compare_w(src, vfs_nthome, vfs_nthome_len) != 0) {
        return S_FALSE;
    }

    /* Check if the character after vfs_nthome is a separator or the end of
       the string */

    if (!path_is_separator_w(src[vfs_nthome_len]) &&
            src[vfs_nthome_len] != L'\0')
    {
        return S_FALSE;
    }

    /* Cut off the matched <prefix>\, add the replaced prefix, count NUL */

    shift = path_is_separator_w(src[vfs_nthome_len]) ? 1 : 0;
    redir_len = wcslen(vfs_nthome_real);
    required = wcslen(src) - vfs_nthome_len - shift + redir_len + 1;

    if (dest != NULL) {
        if (required > *count) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        wcscpy_s(dest, *count, vfs_nthome_real);
        wcscpy_s(dest + redir_len, *count - redir_len, src + vfs_nthome_len + shift);
    }

    *count = required;

    return S_OK;
}

static HRESULT vfs_path_hook_option(
        const wchar_t *src,
        wchar_t *dest,
        size_t *count)
{
    size_t required;
    size_t redir_len;
    size_t shift;

    assert(src != NULL);
    assert(count != NULL);

    /* Case-insensitive check to see if src starts with vfs_option */

    if (path_compare_w(src, vfs_option, vfs_option_len) != 0) {
        return S_FALSE;
    }

    /* Check if the character after vfs_nthome is a separator or the end of
       the string */

    if (!path_is_separator_w(src[vfs_option_len]) &&
            src[vfs_option_len] != L'\0')
    {
        return S_FALSE;
    }

    /* Cut off the matched <prefix>\, add the replaced prefix, count NUL */

    shift = path_is_separator_w(src[vfs_option_len]) ? 1 : 0;
    redir_len = wcslen(vfs_config.option);
    required = wcslen(src) - vfs_option_len - shift + redir_len + 1;

    if (dest != NULL) {
        if (required > *count) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        wcscpy_s(dest, *count, vfs_config.option);
        wcscpy_s(dest + redir_len, *count - redir_len, src + vfs_option_len + shift);
    }

    *count = required;

    return S_OK;
}

static HRESULT vfs_reg_read_amfs(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, vfs_config.amfs);
}

static HRESULT vfs_reg_read_appdata(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, vfs_config.appdata);
}
