#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hook/table.h"

#include "platform/pcbid.h"

#include "util/dprintf.h"

static BOOL WINAPI pcbid_GetComputerNameA(char *dest, uint32_t *len);

static struct pcbid_config pcbid_cfg;

static const struct hook_symbol pcbid_syms[] = {
    {
        .name   = "GetComputerNameA",
        .patch  = pcbid_GetComputerNameA,
    }
};

HRESULT pcbid_hook_init(const struct pcbid_config *cfg)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    if (wcslen(cfg->serial_no) != 15) {
        dprintf("Pcbid: ERROR: Must be 15 chars! ex: ACAE01A99999999\n");

        return E_INVALIDARG;
    }

    memcpy(&pcbid_cfg, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "kernel32.dll", pcbid_syms, _countof(pcbid_syms));

    return S_OK;
}

static BOOL WINAPI pcbid_GetComputerNameA(char *dest, uint32_t *len)
{
    size_t required;

    if (dest == NULL || len == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    wcstombs_s(&required, NULL, 0, pcbid_cfg.serial_no, 0);

    if (required > *len) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

        return FALSE;
    }

    dprintf("Pcbid: Get PCB serial\n");

    wcstombs_s(NULL, dest, *len, pcbid_cfg.serial_no, *len - 1);
    SetLastError(ERROR_SUCCESS);
    *len = required - 1;

    return TRUE;
}
