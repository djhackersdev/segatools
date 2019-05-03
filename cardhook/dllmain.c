#include <windows.h>

#include "board/sg-reader.h"

#include "hook/process.h"

#include "hooklib/serial.h"

#include "util/dprintf.h"
#include "util/spike.h"

static process_entry_t app_startup;

static DWORD CALLBACK app_pre_startup(void)
{
    dprintf("--- Begin %s ---\n", __func__);

    spike_hook_init("cardspike.txt");

    serial_hook_init();
    sg_reader_hook_init(12);

    dprintf("---  End  %s ---\n", __func__);

    return app_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hr = process_hijack_startup(app_pre_startup, &app_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
