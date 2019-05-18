#include <windows.h>

#include <assert.h>
#include <stddef.h>

#include "amex/config.h"

#include "chunihook/config.h"

#include "platform/config.h"

void chuni_hook_config_load(
        struct chuni_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    memset(cfg, 0, sizeof(*cfg));

    nu_config_load(&cfg->nu, filename);
    amex_config_load(&cfg->amex, filename);
}
