#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "amex/nvram.h"

#include "util/dprintf.h"

HRESULT nvram_open_file(HANDLE *out, const wchar_t *path, size_t size)
{
    LARGE_INTEGER cur_size;
    LARGE_INTEGER pos;
    HANDLE file;
    HRESULT hr;
    BOOL ok;

    assert(out != NULL);
    assert(path != NULL);

    *out = NULL;

    file = CreateFileW(
            path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (file == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("%S: Error opening backing store: %x\n", path, (int) hr);

        goto end;
    }

    ok = GetFileSizeEx(file, &cur_size);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("%S: GetFileSizeEx failed: %x\n", path, (int) hr);

        goto end;
    }

    if (cur_size.QuadPart != (uint64_t) size) {
        pos.QuadPart = (uint64_t) size;
        ok = SetFilePointerEx(file, pos, NULL, FILE_BEGIN);

        if (!ok) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dprintf("%S: SetFilePointerEx failed: %x\n", path, (int) hr);

            goto end;
        }

        ok = SetEndOfFile(file);

        if (!ok) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dprintf("%S: SetEndOfFile failed: %x\n", path, (int) hr);

            goto end;
        }
    }

    *out = file;
    file = INVALID_HANDLE_VALUE;

    hr = S_OK;

end:
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }

    return hr;
}
