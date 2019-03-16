#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/ds.h"
#include "amex/eeprom.h"
#include "amex/gpio.h"
#include "amex/jvs.h"
#include "amex/sram.h"

#include "divahook/_com10.h"
#include "divahook/jvs.h"
#include "divahook/slider-hook.h"

#include "hook/process.h"

#include "hooklib/serial.h"

#include "platform/hwmon.h"
#include "platform/nusec.h"

#include "util/clock.h"
#include "util/dprintf.h"
#include "util/gfx.h"
#include "util/spike.h"

static process_entry_t diva_startup;

static DWORD CALLBACK diva_pre_startup(void)
{
    dprintf("--- Begin diva_pre_startup ---\n");

    /* Hook Win32 APIs */

    clock_hook_init();
    serial_hook_init();

    /* Initialize platform API emulation */

    hwmon_hook_init();
    nusec_hook_init();

    /* Initialize AMEX emulation */

    ds_hook_init();
    eeprom_hook_init();
    gpio_hook_init();
    jvs_hook_init();
    sram_hook_init();

    /* Initialize Project Diva I/O board emulation */

    com10_hook_init();
    diva_jvs_init();
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
