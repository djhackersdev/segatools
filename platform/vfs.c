#include <windows.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hooklib/path.h"
#include "hooklib/reg.h"

#include "platform/config.h"
#include "platform/vfs.h"

#include "util/dprintf.h"

static void vfs_slashify(wchar_t *path, size_t max_count);
static HRESULT vfs_mkdir_rec(const wchar_t *path);
static HRESULT vfs_path_hook(const wchar_t *src, wchar_t *dest, size_t *count);
static HRESULT vfs_reg_read_amfs(void *bytes, uint32_t *nbytes);
static HRESULT vfs_reg_read_appdata(void *bytes, uint32_t *nbytes);

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
    HRESULT hr;

    assert(config != NULL);

    if (!config->enable) {
        return S_FALSE;
    }

    memcpy(&vfs_config, config, sizeof(*config));

    vfs_slashify(vfs_config.amfs, _countof(vfs_config.amfs));
    vfs_slashify(vfs_config.appdata, _countof(vfs_config.appdata));

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

    hr = path_hook_push(vfs_path_hook);

    if (FAILED(hr)) {
        return hr;
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

static void vfs_slashify(wchar_t *path, size_t max_count)
{
    size_t count;

    assert(path != NULL);

    count = wcslen(path);

    if (path[count - 1] == L'\\' || path[count - 1] == L'/') {
        return;
    }

    if (count + 2 > max_count) {
        abort();
    }

    path[count + 0] = L'\\';
    path[count + 1] = L'\0';
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

    if (src[0] == L'\0' || src[1] != L':' || src[2] != L'\\') {
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

    /* Cut off E:\ prefix, replace with redir path, count NUL terminator */

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

static HRESULT vfs_reg_read_amfs(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, vfs_config.amfs);
}

static HRESULT vfs_reg_read_appdata(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_wstr(bytes, nbytes, vfs_config.appdata);
}
