#include <windows.h>

#include <stdlib.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "divahook/config.h"
#include "divahook/diva-dll.h"
#include "divahook/jvs.h"
#include "divahook/slider.h"

#include "hook/process.h"

#include "hooklib/gfx.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE diva_hook_mod;
static process_entry_t diva_startup;
static struct diva_hook_config diva_hook_cfg;

static DWORD CALLBACK diva_pre_startup(void)
{
    HRESULT hr;

    dprintf("--- Begin diva_pre_startup ---\n");

    /* Config load */

    diva_hook_config_load(&diva_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    serial_hook_init();

    /* Initialize emulation hooks */

    hr = platform_hook_init(
            &diva_hook_cfg.platform,
            "SBZV",
            "AAV0",
            diva_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = diva_dll_init(&diva_hook_cfg.dll, diva_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = amex_hook_init(&diva_hook_cfg.amex, diva_jvs_init);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = sg_reader_hook_init(&diva_hook_cfg.aime, 10, diva_hook_mod);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = slider_hook_init(&diva_hook_cfg.slider);

    if (FAILED(hr)) {
        goto fail;
    }

    /* Initialize debug helpers */

    spike_hook_init(L".\\segatools.ini");

    dprintf("---  End  diva_pre_startup ---\n");

    /* Jump to EXE start address */

    return diva_startup();

fail:
    ExitProcess(EXIT_FAILURE);
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    diva_hook_mod = mod;

    hr = process_hijack_startup(diva_pre_startup, &diva_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
