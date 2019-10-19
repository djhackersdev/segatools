#include <windows.h>

#include "amex/config.h"
#include "amex/ds.h"

#include "hook/process.h"

#include "hooklib/spike.h"

#include "platform/clock.h"
#include "platform/config.h"
#include "platform/nusec.h"

#include "util/dprintf.h"

static process_entry_t app_startup;

static DWORD CALLBACK app_pre_startup(void)
{
    struct clock_config clock_cfg;
    struct ds_config ds_cfg;
    struct nusec_config nusec_cfg;

    dprintf("--- Begin %s ---\n", __func__);

    clock_config_load(&clock_cfg, L".\\segatools.ini");
    ds_config_load(&ds_cfg, L".\\segatools.ini");
    nusec_config_load(&nusec_cfg, L".\\segatools.ini");
    spike_hook_init(L".\\segatools.ini");

    clock_hook_init(&clock_cfg);
    nusec_hook_init(&nusec_cfg, "SSSS", "AAV0");
    ds_hook_init(&ds_cfg);

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
