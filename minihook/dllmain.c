#include <windows.h>

#include "amex/config.h"
#include "amex/ds.h"

#include "hook/process.h"

#include "hooklib/clock.h"
#include "hooklib/spike.h"

#include "platform/nusec.h"

#include "util/dprintf.h"

static process_entry_t app_startup;

static DWORD CALLBACK app_pre_startup(void)
{
    struct ds_config ds_cfg;

    dprintf("--- Begin %s ---\n", __func__);

    ds_config_load(&ds_cfg, L".\\segatools.ini");
    clock_hook_init();
    ds_hook_init(&ds_cfg);
    nusec_hook_init();

    spike_hook_init("minispike.txt");

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
