#include <assert.h>
#include <stddef.h>

#include "amex/config.h"

#include "board/config.h"

#include "divahook/config.h"

#include "platform/config.h"

void diva_hook_config_load(
        struct diva_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    nu_config_load(&cfg->nu, filename);
    amex_config_load(&cfg->amex, filename);
    aime_config_load(&cfg->aime, filename);
}
