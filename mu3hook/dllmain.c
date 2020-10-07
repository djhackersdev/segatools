#include <windows.h>

#include <stdlib.h>

#include "board/io4.h"
#include "board/sg-reader.h"
#include "board/vfd.h"

#include "hook/process.h"

#include "hooklib/dvd.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "mu3hook/config.h"
#include "mu3hook/io4.h"
#include "mu3hook/mu3-dll.h"
#include "mu3hook/unity.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE mu3_hook_mod;
static process_entry_t mu3_startup;
static struct mu3_hook_config mu3_hook_cfg;

static DWORD CALLBACK mu3_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin mu3_pre_startup ---\n");

    /* Load config */

    mu3_hook_config_load(&mu3_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    dvd_hook_init(&mu3_hook_cfg.dvd, mu3_hook_mod);
    gfx_hook_init(&mu3_hook_cfg.gfx, mu3_hook_mod);
    serial_hook_init();

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &mu3_hook_cfg.platform,
            "SDDT",
            "ACA1",
            mu3_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&mu3_hook_cfg.aime, 1, mu3_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = vfd_hook_init(2);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = mu3_dll_init(&mu3_hook_cfg.dll, mu3_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = mu3_io4_hook_init(&mu3_hook_cfg.io4);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Initialize Unity native plugin DLL hooks

       There seems to be an issue with other DLL hooks if `LoadLibraryW` is
       hooked earlier in the `mu3hook` initialization. */

    unity_hook_init();

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  mu3_pre_startup ---\n");

    /* Jump to EXE start address */

    return mu3_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    mu3_hook_mod = mod;

    hr = process_hijack_startup(mu3_pre_startup, &mu3_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
