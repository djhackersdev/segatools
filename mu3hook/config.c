#include <assert.h>
#include <stddef.h>

#include "board/config.h"

#include "hooklib/config.h"
#include "hooklib/gfx.h"

#include "mu3hook/config.h"

#include "platform/config.h"

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    platform_config_load(&cfg->platform, filename);
    aime_config_load(&cfg->aime, filename);
    gfx_config_load(&cfg->gfx, filename);
}
