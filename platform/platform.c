#include <windows.h>

#include <assert.h>

#include "platform/amvideo.h"
#include "platform/config.h"
#include "platform/hwmon.h"
#include "platform/nusec.h"
#include "platform/platform.h"
#include "platform/vfs.h"

HRESULT platform_hook_init_nu(
        const struct nu_config *cfg,
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

    hr = hwmon_hook_init(&cfg->hwmon);

    if (FAILED(hr)) {
        return hr;
    }

    hr = nusec_hook_init(&cfg->nusec, game_id, platform_id);

    if (FAILED(hr)) {
        return hr;
    }

    hr = vfs_hook_init(&cfg->vfs);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}
