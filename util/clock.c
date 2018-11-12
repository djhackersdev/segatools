#include <windows.h>

#include <stdint.h>

#include "hook/table.h"

#include "util/dprintf.h"

static void WINAPI my_GetSystemTimeAsFileTime(FILETIME *out);
static BOOL WINAPI my_GetLocalTime(SYSTEMTIME *out);
static BOOL WINAPI my_GetSystemTime(SYSTEMTIME *out);
static DWORD WINAPI my_GetTimeZoneInformation(TIME_ZONE_INFORMATION *tzinfo);
static BOOL WINAPI my_SetSystemTime(SYSTEMTIME *in);
static BOOL WINAPI my_SetTimeZoneInformation(TIME_ZONE_INFORMATION *tzinfo);

static BOOL (WINAPI * next_GetSystemTimeAsFileTime)(FILETIME *out);
static int64_t clock_current_day;

static const struct hook_symbol clock_skew_hook_syms[] = {
    /* Canonical time */

    {
        .name   = "GetSystemTimeAsFileTime",
        .patch  = my_GetSystemTimeAsFileTime,
        .link   = (void **) &next_GetSystemTimeAsFileTime,
    },

    /* Derived time */

    {
        .name   = "GetLocalTime",
        .patch  = my_GetLocalTime,
    }, {
        .name   = "GetSystemTime",
        .patch  = my_GetSystemTime,
    }, {
        .name   = "GetTimeZoneInformation",
        .patch  = my_GetTimeZoneInformation,
    },
};

static const struct hook_symbol clock_set_hook_syms[] = {
    {
        .name   = "SetSystemTime",
        .patch  = my_SetSystemTime,
    }, {
        .name   = "SetTimeZoneInformation",
        .patch  = my_SetTimeZoneInformation,
    },
};

/* FILETIME is expressed in 100ns i.e. 0.1us i.e. 10^-7 sec units.
   No official name for these units is given so let's call them "jiffies". */

static const int64_t jiffies_per_sec = 10000000LL;
static const int64_t jiffies_per_hour = jiffies_per_sec * 3600LL;
static const int64_t jiffies_per_day = jiffies_per_hour * 24LL;

static void WINAPI my_GetSystemTimeAsFileTime(FILETIME *out)
{
    FILETIME in;
    int64_t day;
    int64_t real_jiffies;
    int64_t real_jiffies_biased;
    int64_t real_time;
    int64_t fake_time;
    int64_t fake_jiffies_biased;
    int64_t fake_jiffies;

    if (out == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return;
    }

    /* Get and convert real jiffies */

    next_GetSystemTimeAsFileTime(&in);
    real_jiffies = (((int64_t) in.dwHighDateTime) << 32) | in.dwLowDateTime;

    /* Apply bias: shift [02:00, 07:00) to [19:00, 24:00) */

    real_jiffies_biased = real_jiffies + 17LL * jiffies_per_hour;

    /* Split date and time */

          day = real_jiffies_biased / jiffies_per_day;
    real_time = real_jiffies_biased % jiffies_per_day;

    /* Debug log */

    if (clock_current_day != 0 && clock_current_day != day) {
        dprintf("\n*** CLOCK JUMP! ***\n\n");
    }

    clock_current_day = day;

    /* We want to skip the final five hours, so scale time-of-day by 19/24. */

    fake_time = (real_time * 19LL) / 24LL;

    /* Un-split date and time */

    fake_jiffies_biased = day * jiffies_per_day + fake_time;

    /* Remove bias */

    fake_jiffies = fake_jiffies_biased - 17LL * jiffies_per_hour;

    /* Return result */

    out->dwLowDateTime  = fake_jiffies;
    out->dwHighDateTime = fake_jiffies >> 32;
}

static BOOL WINAPI my_GetLocalTime(SYSTEMTIME *out)
{
    /* Use UTC to simplify things */
    return my_GetSystemTime(out);
}

static BOOL WINAPI my_GetSystemTime(SYSTEMTIME *out)
{
    FILETIME linear;
    BOOL ok;

    my_GetSystemTimeAsFileTime(&linear);
    ok = FileTimeToSystemTime(&linear, out);

    if (!ok) {
        return ok;
    }

#if 0
    static int last_second;

    if (out->wSecond != last_second) {
        dprintf("%04i/%02i/%02i %02i:%02i:%02i\n",
                out->wYear,
                out->wMonth,
                out->wDay,
                out->wHour,
                out->wMinute,
                out->wSecond);
    }

    last_second = out->wSecond;
#endif

    return TRUE;
}

static DWORD WINAPI my_GetTimeZoneInformation(TIME_ZONE_INFORMATION *tzinfo)
{
    /* Use UTC to simplify things */

    dprintf("%s\n", __func__);

    if (tzinfo != NULL) {
        memset(tzinfo, 0, sizeof(*tzinfo));
        SetLastError(ERROR_SUCCESS);

        return TIME_ZONE_ID_UNKNOWN;
    } else {
        SetLastError(ERROR_INVALID_PARAMETER);

        return TIME_ZONE_ID_INVALID;
    }
}

static BOOL WINAPI my_SetSystemTime(SYSTEMTIME *in)
{
    dprintf("Prevented application from screwing with the system clock\n");

    return TRUE;
}

static BOOL WINAPI my_SetTimeZoneInformation(TIME_ZONE_INFORMATION *in)
{
    dprintf("Prevented application from screwing with the timezone\n");

    return TRUE;
}

void clock_set_hook_init(void)
{
    hook_table_apply(
            NULL,
            "kernel32.dll",
            clock_set_hook_syms,
            _countof(clock_set_hook_syms));
}

void clock_skew_hook_init(void)
{
    hook_table_apply(
            NULL,
            "kernel32.dll",
            clock_skew_hook_syms,
            _countof(clock_skew_hook_syms));
}
