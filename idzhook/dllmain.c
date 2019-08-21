#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"

#include "board/sg-reader.h"

#include "hook/process.h"

#include "hooklib/clock.h"
#include "hooklib/gfx.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "idzhook/config.h"
#include "idzhook/jvs.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE idz_hook_mod;
static process_entry_t idz_startup;
static struct idz_hook_config idz_hook_cfg;

static DWORD CALLBACK idz_pre_startup(void)
{
    dprintf("--- Begin idz_pre_startup ---\n");

    /* Config load */

    idz_hook_config_load(&idz_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    clock_write_hook_init();
    serial_hook_init();

    /* Initialize emulation hooks */

    platform_hook_init_nu(&idz_hook_cfg.nu, "SDDF", "AAV2", idz_hook_mod);
    amex_hook_init(&idz_hook_cfg.amex);

    if (idz_hook_cfg.amex.jvs.enable) {
        idz_jvs_init();
    }

    sg_reader_hook_init(&idz_hook_cfg.aime, 10);

    /* Initialize debug helpers */

    spike_hook_init("idzspike.txt");

    dprintf("---  End  idz_pre_startup ---\n");

    /* Jump to EXE start address */

    return idz_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    idz_hook_mod = mod;

    hr = process_hijack_startup(idz_pre_startup, &idz_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
