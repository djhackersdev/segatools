#include <windows.h>

#include <stdlib.h>

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
    HRESULT hr;

    dprintf("--- Begin %s ---\n", __func__);

    clock_config_load(&clock_cfg, L".\\segatools.ini");
    ds_config_load(&ds_cfg, L".\\segatools.ini");
    nusec_config_load(&nusec_cfg, L".\\segatools.ini");
    spike_hook_init(L".\\segatools.ini");

    hr = clock_hook_init(&clock_cfg);

    if (FAILED(hr)) {
        goto fail;
    }

    hr = nusec_hook_init(&nusec_cfg, "SSSS", "AAV0");

    if (FAILED(hr)) {
        goto fail;
    }

    hr = ds_hook_init(&ds_cfg);

    if (FAILED(hr)) {
        goto fail;
    }

    dprintf("---  End  %s ---\n", __func__);

    return app_startup();

fail:
    ExitProcess(EXIT_FAILURE);
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
