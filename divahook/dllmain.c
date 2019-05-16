#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "board/sg-reader.h"

#include "divahook/jvs.h"
#include "divahook/slider.h"

#include "hook/process.h"

#include "hooklib/clock.h"
#include "hooklib/gfx.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/hwmon.h"
#include "platform/nusec.h"

#include "util/dprintf.h"

static process_entry_t diva_startup;

static DWORD CALLBACK diva_pre_startup(void)
{
    struct amex_config amex_cfg;

    dprintf("--- Begin diva_pre_startup ---\n");

    /* Hook Win32 APIs */

    clock_hook_init();
    serial_hook_init();

    /* Initialize platform API emulation */

    hwmon_hook_init();
    nusec_hook_init();

    /* Initialize AMEX emulation */

    amex_config_load(&amex_cfg, L".\\segatools.ini");
    amex_hook_init(&amex_cfg);

    /* Initialize Project Diva I/O board emulation */

    if (amex_cfg.jvs.enable) {
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

    hr = process_hijack_startup(diva_pre_startup, &diva_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
