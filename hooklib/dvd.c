#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "hooklib/config.h"
#include "hooklib/dll.h"
#include "hooklib/dvd.h"

#include "util/dprintf.h"

/* API hooks */

static DWORD WINAPI hook_QueryDosDeviceW(
    const wchar_t *lpDeviceName,
    wchar_t *lpTargetPath,
    DWORD ucchMax);

/* Link pointers */

static DWORD (WINAPI *next_QueryDosDeviceW)(
    const wchar_t *lpDeviceName,
    wchar_t *lpTargetPath,
    DWORD ucchMax);

static bool dvd_hook_initted;
static struct dvd_config dvd_config;

static const struct hook_symbol dvd_hooks[] = {
    {
        .name   = "QueryDosDeviceW",
        .patch  = hook_QueryDosDeviceW,
        .link   = (void **) &next_QueryDosDeviceW
    },
};

void dvd_hook_init(const struct dvd_config *cfg, HINSTANCE self)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    if (dvd_hook_initted) {
        return;
    }

    dvd_hook_initted = true;

    memcpy(&dvd_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "kernel32.dll", dvd_hooks, _countof(dvd_hooks));
    dprintf("DVD: hook enabled.\n");
}

DWORD WINAPI hook_QueryDosDeviceW(
    const wchar_t *lpDeviceName,
    wchar_t *lpTargetPath,
    DWORD ucchMax)
{
    DWORD ok;
    wchar_t *p_dest;
    wchar_t *dvd_string = L"CdRom";

    ok = next_QueryDosDeviceW(
        lpDeviceName,
        lpTargetPath,
        ucchMax);

    p_dest = wcsstr (lpTargetPath, dvd_string);

    if ( p_dest != NULL ) {
        dprintf("DVD: Hiding DVD drive.\n");
        return 0;
    }

    return ok;
}
