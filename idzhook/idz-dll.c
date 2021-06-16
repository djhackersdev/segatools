#include <windows.h>

#include <assert.h>
#include <stdlib.h>

#include "idzhook/idz-dll.h"

#include "util/dll-bind.h"
#include "util/dprintf.h"

const struct dll_bind_sym idz_dll_syms[] = {
    {
        .sym = "idz_io_jvs_init",
        .off = offsetof(struct idz_dll, jvs_init),
    }, {
        .sym = "idz_io_jvs_read_analogs",
        .off = offsetof(struct idz_dll, jvs_read_analogs),
    }, {
        .sym = "idz_io_jvs_read_buttons",
        .off = offsetof(struct idz_dll, jvs_read_buttons),
    }, {
        .sym = "idz_io_jvs_read_shifter",
        .off = offsetof(struct idz_dll, jvs_read_shifter),
    }, {
        .sym = "idz_io_jvs_read_coin_counter",
        .off = offsetof(struct idz_dll, jvs_read_coin_counter),
    }
};

struct idz_dll idz_dll;

// Copypasta DLL binding and diagnostic message boilerplate.
// Not much of this lends itself to being easily factored out. Also there
// will be a lot of API-specific branching code here eventually as new API
// versions get defined, so even though these functions all look the same
// now this won't remain the case forever.

HRESULT idz_dll_init(const struct idz_dll_config *cfg, HINSTANCE self)
{
    uint16_t (*get_api_version)(void);
    const struct dll_bind_sym *sym;
    HINSTANCE owned;
    HINSTANCE src;
    HRESULT hr;

    assert(cfg != NULL);
    assert(self != NULL);

    if (cfg->path[0] != L'\0') {
        owned = LoadLibraryW(cfg->path);

        if (owned == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dprintf("IDZ IO: Failed to load IO DLL: %lx: %S\n",
                    hr,
                    cfg->path);

            goto end;
        }

        dprintf("IDZ IO: Using custom IO DLL: %S\n", cfg->path);
        src = owned;
    } else {
        owned = NULL;
        src = self;
    }

    get_api_version = (void *) GetProcAddress(src, "idz_io_get_api_version");

    if (get_api_version != NULL) {
        idz_dll.api_version = get_api_version();
    } else {
        idz_dll.api_version = 0x0100;
        dprintf("Custom IO DLL does not expose idz_io_get_api_version, "
                "assuming API version 1.0.\n"
                "Please ask the developer to update their DLL.\n");
    }

    if (idz_dll.api_version >= 0x0200) {
        hr = E_NOTIMPL;
        dprintf("IDZ IO: Custom IO DLL implements an unsupported "
                "API version (%#04x). Please update Segatools.\n",
                idz_dll.api_version);

        goto end;
    }

    sym = idz_dll_syms;
    hr = dll_bind(&idz_dll, src, &sym, _countof(idz_dll_syms));

    if (FAILED(hr)) {
        if (src != self) {
            dprintf("IDZ IO: Custom IO DLL does not provide function "
                    "\"%s\". Please contact your IO DLL's developer for "
                    "further assistance.\n",
                    sym->sym);

            goto end;
        } else {
            dprintf("Internal error: could not reflect \"%s\"\n", sym->sym);
        }
    }

    owned = NULL;

end:
    if (owned != NULL) {
        FreeLibrary(owned);
    }

    return hr;
}
