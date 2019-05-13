#pragma once

#include <stdbool.h>
#include <string.h>
#include <wchar.h>

inline bool str_eq(const char *lhs, const char *rhs)
{
    if (lhs == NULL || rhs == NULL) {
        return lhs == rhs;
    }

    return strcmp(lhs, rhs) == 0;
}

inline bool str_ieq(const char *lhs, const char *rhs)
{
    if (lhs == NULL || rhs == NULL) {
        return lhs == rhs;
    }

    return _stricmp(lhs, rhs) == 0;
}

inline bool wstr_eq(const wchar_t *lhs, const wchar_t *rhs)
{
    if (lhs == NULL || rhs == NULL) {
        return lhs == rhs;
    }

    return wcscmp(lhs, rhs) == 0;
}

inline bool wstr_ieq(const wchar_t *lhs, const wchar_t *rhs)
{
    if (lhs == NULL || rhs == NULL) {
        return lhs == rhs;
    }

    return _wcsicmp(lhs, rhs) == 0;
}
