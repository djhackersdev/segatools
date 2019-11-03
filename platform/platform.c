#include <windows.h>

#include <assert.h>

#include "platform/amvideo.h"
#include "platform/clock.h"
#include "platform/config.h"
#include "platform/dns.h"
#include "platform/hwmon.h"
#include "platform/misc.h"
#include "platform/netenv.h"
#include "platform/nusec.h"
#include "platform/pcbid.h"
#include "platform/platform.h"
#include "platform/vfs.h"

HRESULT platform_hook_init(
        const struct platform_config *cfg,
        const char *game_id,
        const char *platform_id,
        HMODULE redir_mod)
{
    HRESULT hr;

    assert(cfg != NULL);
    assert(game_id != NULL);
    assert(platform_id != NULL);
    assert(redir_mod != NULL);

    hr = amvideo_hook_init(&cfg->amvideo, redir_mod);

    if (FAILED(hr)) {
        return hr;
    }

    hr = clock_hook_init(&cfg->clock);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_platform_hook_init(&cfg->dns);

    if (FAILED(hr)) {
        return hr;
    }

    hr = hwmon_hook_init(&cfg->hwmon);

    if (FAILED(hr)) {
        return hr;
    }

    hr = misc_hook_init(&cfg->misc, platform_id);

    if (FAILED(hr)) {
        return hr;
    }

    hr = netenv_hook_init(&cfg->netenv, &cfg->nusec);

    if (FAILED(hr)) {
        return hr;
    }

    hr = nusec_hook_init(&cfg->nusec, game_id, platform_id);

    if (FAILED(hr)) {
        return hr;
    }

    pcbid_hook_init(&cfg->pcbid);

    hr = vfs_hook_init(&cfg->vfs);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}
