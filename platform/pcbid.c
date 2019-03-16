#include <windows.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hook/table.h"

#include "util/dprintf.h"

static BOOL WINAPI pcbid_GetComputerNameA(char *dest, uint32_t *len);

static const char pcbid_file[] = "DEVICE/pcbid.txt";
static char pcbid_str[16];

static const struct hook_symbol pcbid_syms[] = {
    {
        .name   = "GetComputerNameA",
        .patch  = pcbid_GetComputerNameA,
    }
};

void pcbid_hook_init(void)
{
    FILE *f;

    f = fopen(pcbid_file, "r");

    if (f != NULL) {
        /* De-hyphenate the serial number. Game code will re-insert it. */
        fscanf(f, "%4s-", &pcbid_str[0]);
        fscanf(f, "%11s", &pcbid_str[4]);
        fclose(f);
    } else {
        dprintf("Failed to open %s\n", pcbid_file);
    }

    hook_table_apply(NULL, "kernel32.dll", pcbid_syms, _countof(pcbid_syms));
}

static BOOL WINAPI pcbid_GetComputerNameA(char *dest, uint32_t *len)
{
    if (dest == NULL || len == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

    if (*len < sizeof(pcbid_str)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);

        return FALSE;
    }

    dprintf("Pcbid: Get PCB serial\n");

    memcpy(dest, pcbid_str, sizeof(pcbid_str));
    *len = sizeof(pcbid_str) - 1;

    return TRUE;
}
