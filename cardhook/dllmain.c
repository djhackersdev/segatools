#include <windows.h>

#include <stdbool.h>

#include "board/config.h"
#include "board/sg-reader.h"

#include "hook/process.h"

#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/config.h"
#include "platform/dns.h"

#include "util/dprintf.h"

static struct aime_config app_aime_config;
static struct dns_config app_dns_config;
static process_entry_t app_startup;

static DWORD CALLBACK app_pre_startup(void)
{
    dprintf("--- Begin %s ---\n", __func__);

    aime_config_load(&app_aime_config, L".\\segatools.ini");
    dns_config_load(&app_dns_config, L".\\segatools.ini");

    serial_hook_init();
    sg_reader_hook_init(&app_aime_config, 12);
    dns_platform_hook_init(&app_dns_config);

    spike_hook_init("cardspike.txt");

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
