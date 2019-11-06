#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "hook/table.h"

#include "platform/clock.h"

#include "util/dprintf.h"

static void WINAPI my_GetSystemTimeAsFileTime(FILETIME *out);
static BOOL WINAPI my_GetLocalTime(SYSTEMTIME *out);
static BOOL WINAPI my_GetSystemTime(SYSTEMTIME *out);
static DWORD WINAPI my_GetTimeZoneInformation(TIME_ZONE_INFORMATION *tzinfo);
static BOOL WINAPI my_SetLocalTime(SYSTEMTIME *in);
static BOOL WINAPI my_SetSystemTime(SYSTEMTIME *in);
static BOOL WINAPI my_SetTimeZoneInformation(TIME_ZONE_INFORMATION *tzinfo);

static BOOL (WINAPI * next_GetSystemTimeAsFileTime)(FILETIME *out);
static int64_t clock_current_day;
static bool clock_time_warp;

static const struct hook_symbol clock_base_hook_syms[] = {
    {
        .name   = "GetSystemTimeAsFileTime",
        .patch  = my_GetSystemTimeAsFileTime,
        .link   = (void **) &next_GetSystemTimeAsFileTime,
    }
};

static const struct hook_symbol clock_read_hook_syms[] = {
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

static const struct hook_symbol clock_write_hook_syms[] = {
    {
        .name   = "SetLocalTime",
        .patch  = my_SetLocalTime,
    }, {
        .name   = "SetSystemTime",
        .patch  = my_SetSystemTime,
    }, {
        .name   = "SetTimeZoneInformation",
        .patch  = my_SetTimeZoneInformation,
    },
};

/* FILETIME is expressed in 100ns i.e. 0.1us i.e. 10^-7 sec units.
   No official name for these units is given so let's call them "jiffies". */

#define jiffies_per_sec     10000000LL
#define jiffies_per_hour    (jiffies_per_sec * 3600LL)
#define jiffies_per_day     (jiffies_per_hour * 24LL)

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

    if (!clock_time_warp) {
        next_GetSystemTimeAsFileTime(out);

        return;
    }

    if (out == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return;
    }

    /* Get and convert real jiffies */

    next_GetSystemTimeAsFileTime(&in);
    real_jiffies = (((int64_t) in.dwHighDateTime) << 32) | in.dwLowDateTime;

    /* Keepout period is JST [02:00, 07:00), which is equivalent to
       UTC [17:00, 22:00). Bias UTC forward by 2 hours, changing this interval
       to [19:00, 00:00) to make the math easier. We revert this bias later. */

    real_jiffies_biased = real_jiffies + 2LL * jiffies_per_hour;

    /* Split date and time */

          day = real_jiffies_biased / jiffies_per_day;
    real_time = real_jiffies_biased % jiffies_per_day;

    /* Debug log */

    if (clock_current_day != 0 && clock_current_day != day) {
        dprintf("\n*** CLOCK JUMP! ***\n\n");
    }

    clock_current_day = day;

    /* We want to skip the final five hours of our UTC+2 biased reference frame,
       so scale time-of-day by 19/24. */

    fake_time = (real_time * 19LL) / 24LL;

    /* Un-split date and time */

    fake_jiffies_biased = day * jiffies_per_day + fake_time;

    /* Revert bias */

    fake_jiffies = fake_jiffies_biased - 2LL * jiffies_per_hour;

    /* Return result */

    out->dwLowDateTime  = fake_jiffies;
    out->dwHighDateTime = fake_jiffies >> 32;
}

static BOOL WINAPI my_GetLocalTime(SYSTEMTIME *out)
{
    ULARGE_INTEGER arith;
    FILETIME linear;

    /* Force JST */

    my_GetSystemTimeAsFileTime(&linear);

    arith.LowPart = linear.dwLowDateTime;
    arith.HighPart = linear.dwHighDateTime;
    arith.QuadPart += 9ULL * jiffies_per_hour;
    linear.dwLowDateTime = arith.LowPart;
    linear.dwHighDateTime = arith.HighPart;

    return FileTimeToSystemTime(&linear, out);
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
    dprintf("Clock: Returning JST timezone\n");

    if (tzinfo == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return TIME_ZONE_ID_INVALID;
    }

    /* Force JST (UTC+9), SEGA games malfunction in any other time zone.
       Strings and boundary times don't matter, we only set the offset. */

    memset(tzinfo, 0, sizeof(*tzinfo));
    tzinfo->Bias = -9 * 60;

    SetLastError(ERROR_SUCCESS);

    /* "Unknown" here means that this region does not observe DST */

    return TIME_ZONE_ID_UNKNOWN;
}

static BOOL WINAPI my_SetLocalTime(SYSTEMTIME *in)
{
    dprintf("Clock: Blocked local time update\n");

    return TRUE;
}

static BOOL WINAPI my_SetSystemTime(SYSTEMTIME *in)
{
    dprintf("Clock: Blocked system time update\n");

    return TRUE;
}

static BOOL WINAPI my_SetTimeZoneInformation(TIME_ZONE_INFORMATION *in)
{
    dprintf("Clock: Blocked timezone update\n");

    return TRUE;
}

HRESULT clock_hook_init(const struct clock_config *cfg)
{
    assert(cfg != NULL);

    clock_time_warp = cfg->timewarp;

    if (cfg->timezone || cfg->timewarp || !cfg->writeable) {
        /* All the clock hooks require the core GSTAFT hook to be installed */
        /* Note the ! up there btw. */

        hook_table_apply(
                NULL,
                "kernel32.dll",
                clock_base_hook_syms,
                _countof(clock_base_hook_syms));
    }

    if (cfg->timezone) {
        hook_table_apply(
                NULL,
                "kernel32.dll",
                clock_read_hook_syms,
                _countof(clock_read_hook_syms));
    }

    if (!cfg->writeable) {
        /* Install hook if this config parameter is FALSE! */
        hook_table_apply(
                NULL,
                "kernel32.dll",
                clock_write_hook_syms,
                _countof(clock_write_hook_syms));
    }

    return S_OK;
}
