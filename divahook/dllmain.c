#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "divahook/config.h"
#include "divahook/jvs.h"
#include "divahook/slider.h"

#include "hook/process.h"

#include "hooklib/clock.h"
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
    dprintf("--- Begin diva_pre_startup ---\n");

    /* Config load */

    diva_hook_config_load(&diva_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    clock_hook_init();
    serial_hook_init();

    /* Initialize emulation hooks */

    platform_hook_init_nu(&diva_hook_cfg.nu, "SBZV", "AAV0", diva_hook_mod);
    amex_hook_init(&diva_hook_cfg.amex);

    if (diva_hook_cfg.amex.jvs.enable) {
        diva_jvs_init();
    }

    sg_reader_hook_init(10);
    slider_hook_init();

    /* Initialize debug helpers */

    spike_hook_init("divaspike.txt");

    dprintf("---  End  diva_pre_startup ---\n");

    /* Jump to EXE start address */

    return diva_startup();
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
