#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"
#include "amex/cfg.h"

#include "board/sg-reader.h"

#include "hook/process.h"

#include "hooklib/serial.h"

#include "idzhook/jvs.h"

#include "platform/hwmon.h"
#include "platform/nusec.h"

#include "util/clock.h"
#include "util/dprintf.h"
#include "util/gfx.h"
#include "util/spike.h"

static process_entry_t idz_startup;

static DWORD CALLBACK idz_pre_startup(void)
{
    struct amex_config amex_cfg;

    dprintf("--- Begin idz_pre_startup ---\n");

    /* Hook Win32 APIs */

    clock_hook_init();
    serial_hook_init();

    /* Initialize platform API emulation */

    hwmon_hook_init();
    nusec_hook_init();

    /* Initialize AMEX emulation */

    amex_config_load(&amex_cfg, L".\\segatools.ini");
    amex_hook_init(&amex_cfg);

    /* Initialize Initial D Zero I/O board emulation */

    sg_reader_hook_init(10);

    if (amex_cfg.jvs.enable) {
        idz_jvs_init();
    }

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

    hr = process_hijack_startup(idz_pre_startup, &idz_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
