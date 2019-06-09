#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"

#include "board/config.h"
#include "board/sg-reader.h"

#include "chunihook/config.h"
#include "chunihook/jvs.h"
#include "chunihook/slider.h"

#include "chuniio/chuniio.h"

#include "hook/process.h"

#include "hooklib/clock.h"
#include "hooklib/gfx.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/platform.h"

#include "util/dprintf.h"

static HMODULE chuni_hook_mod;
static process_entry_t chuni_startup;
static struct chuni_hook_config chuni_hook_cfg;

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

    /* Config load */

    chuni_hook_config_load(&chuni_hook_cfg, L".\\segatools.ini");

    /* Hook Win32 APIs */

    clock_hook_init();
    gfx_hook_init();
    serial_hook_init();

    /* Initialize emulation hooks */

    platform_hook_init_nu(&chuni_hook_cfg.nu, "SDBT", "AAV1", chuni_hook_mod);
    amex_hook_init(&chuni_hook_cfg.amex);
    sg_reader_hook_init(&chuni_hook_cfg.aime, 12);

    if (chuni_hook_cfg.amex.jvs.enable) {
        chunithm_jvs_init();
    }

    slider_hook_init();

    /* Initialize debug helpers */

    spike_hook_init("chunispike.txt");
    gfx_set_windowed();

    dprintf("---  End  chuni_pre_startup ---\n");

    /* Initialize IO DLL */

    chuni_io_init();

    /* Jump to EXE start address */

    return chuni_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    chuni_hook_mod = mod;

    hr = process_hijack_startup(chuni_pre_startup, &chuni_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
