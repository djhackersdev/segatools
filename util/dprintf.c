#ifndef NDEBUG

#include <windows.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/dprintf.h"

static char dbg_buf[16384];
static size_t dbg_buf_pos;

void dprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    dprintfv(fmt, ap);
    va_end(ap);
}

void dprintfv(const char *fmt, va_list ap)
{
    dbg_buf_pos += vsnprintf_s(
            dbg_buf + dbg_buf_pos,
            sizeof(dbg_buf) - dbg_buf_pos,
            sizeof(dbg_buf) - dbg_buf_pos - 1,
            fmt,
            ap);

    if (dbg_buf_pos + 1 > sizeof(dbg_buf)) {
        abort();
    }

    if (strchr(dbg_buf, '\n') == NULL) {
        return;
    }

    OutputDebugStringA(dbg_buf);
    dbg_buf_pos = 0;
    dbg_buf[0] = '\0';
}

void dwprintf(const wchar_t *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    dwprintfv(fmt, ap);
    va_end(ap);
}

void dwprintfv(const wchar_t *fmt, va_list ap)
{
    wchar_t msg[512];

    _vsnwprintf_s(msg, _countof(msg), _countof(msg) - 1, fmt, ap);
    OutputDebugStringW(msg);
}

#endif
