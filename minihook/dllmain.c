#include <windows.h>

#include "amex/config.h"
#include "amex/ds.h"

#include "hook/process.h"

#include "hooklib/clock.h"
#include "hooklib/spike.h"

#include "platform/config.h"
#include "platform/nusec.h"

#include "util/dprintf.h"

static process_entry_t app_startup;

static DWORD CALLBACK app_pre_startup(void)
{
    struct nusec_config nusec_cfg;
    struct ds_config ds_cfg;

    dprintf("--- Begin %s ---\n", __func__);

    nusec_config_load(&nusec_cfg, L".\\segatools.ini");
    ds_config_load(&ds_cfg, L".\\segatools.ini");

    // TODO make use of clock read hook configurable
    clock_read_hook_init();
    clock_write_hook_init();
    nusec_hook_init(&nusec_cfg, "SSSS", "AAV0");
    ds_hook_init(&ds_cfg);

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
