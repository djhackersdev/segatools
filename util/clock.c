#include "hook/table.h"

#include "util/dprintf.h"

static BOOL WINAPI my_SetSystemTime(void *whatever);
static BOOL WINAPI my_SetTimeZoneInformation(void *whatever);

static const struct hook_symbol clock_hook_syms[] = {
    {
        .name   = "SetSystemTime",
        .patch  = my_SetSystemTime,
    }, {
        .name   = "SetTimeZoneInformation",
        .patch  = my_SetTimeZoneInformation,
    }
};

void clock_hook_init(void)
{
    hook_table_apply(
            NULL,
            "kernel32.dll",
            clock_hook_syms,
            _countof(clock_hook_syms));
}

static BOOL WINAPI my_SetSystemTime(void *whatever)
{
    dprintf("Prevented application from screwing with the system clock\n");

    return TRUE;
}

static BOOL WINAPI my_SetTimeZoneInformation(void *whatever)
{
    dprintf("Prevented application from screwing with the timezone\n");

    return TRUE;
}
