#include <assert.h>
#include <stddef.h>

#include "board/config.h"

#include "mu3hook/config.h"

#include "platform/config.h"

void mu3_hook_config_load(
        struct mu3_hook_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    alls_config_load(&cfg->alls, filename);
    aime_config_load(&cfg->aime, filename);
}
