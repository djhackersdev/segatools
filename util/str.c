#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>

#include "util/str.h"

extern inline bool str_eq(const char *lhs, const char *rhs);
extern inline bool str_ieq(const char *lhs, const char *rhs);
extern inline bool wstr_eq(const wchar_t *lhs, const wchar_t *rhs);
extern inline bool wstr_ieq(const wchar_t *lhs, const wchar_t *rhs);
