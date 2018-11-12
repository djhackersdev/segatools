#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "chunihook/jvs.h"
#include "chunihook/slider.h"

#include "hook/process.h"

#include "hooklib/serial.h"

#include "nu/ds.h"
#include "nu/eeprom.h"
#include "nu/gpio.h"
#include "nu/jvs.h"
#include "nu/hwmon.h"
#include "nu/nusec.h"
#include "nu/sram.h"

#include "util/clock.h"
#include "util/dprintf.h"
#include "util/gfx.h"
#include "util/spike.h"

static process_entry_t chuni_startup;

static DWORD CALLBACK chuni_pre_startup(void)
{
    HMODULE d3dc;

    dprintf("--- Begin chuni_pre_startup ---\n");

    /* Pin the D3D shader compiler. This makes startup much faster. */

    d3dc = LoadLibraryW(L"D3DCompiler_43.dll");

    if (d3dc != NULL) {
        dprintf("Pinned shader compiler, hMod=%p\n", d3dc);
    } else {
        dprintf("Failed to load shader compiler!\n");
    }

    /* Hook Win32 APIs */

    clock_hook_init();
    gfx_hook_init();
    serial_hook_init();

    /* Initialize Nu emulation */

    eeprom_hook_init();
    sram_hook_init();
    ds_hook_init();
    gpio_hook_init();
    jvs_hook_init();
    hwmon_hook_init();
    nusec_hook_init();

    /* Initialize Chunithm board emulation */

    slider_hook_init();
    chunithm_jvs_init();

    /* Initialize debug helpers */

    spike_hook_init("chunispike.txt");
    gfx_set_windowed();

    dprintf("---  End  chuni_pre_startup ---\n");

    /* Jump to EXE start address */

    return chuni_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hr = process_hijack_startup(chuni_pre_startup, &chuni_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
