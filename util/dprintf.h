#pragma once

#include <stdarg.h>
#include <stddef.h>

#ifdef __GNUC__
#define DPRINTF_CHK __attribute__(( format(printf, 1, 2) ))
#else
#define DPRINTF_CHK
#endif

#ifndef NDEBUG
void dprintf(const char *fmt, ...) DPRINTF_CHK;
void dprintfv(const char *fmt, va_list ap);
void dwprintf(const wchar_t *fmt, ...);
void dwprintfv(const wchar_t *fmt, va_list ap);
#else
#define dprintf(...)
#define dprintfv(fmt, ap)
#define dwprintf(...)
#define dwprintfv(fmt, ap)
#endif
