#include <windows.h>

#include <string.h>
#include <stdlib.h>

#include "hook/table.h"

#include "hooklib/dll.h"

#include "util/dprintf.h"

/* Hook functions */

static int amDllVideoOpen(void *ctx);
static int amDllVideoClose(void *ctx);
static int amDllVideoSetResolution(void *ctx, void *param);
static int amDllVideoGetVBiosVersion(void *ctx, char *dest, size_t nchars);

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

HRESULT amvideo_hook_init(HMODULE redir_mod)
{
    return dll_hook_push(
            redir_mod,
            L"$amvideo",
            amvideo_syms,
            _countof(amvideo_syms));
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
