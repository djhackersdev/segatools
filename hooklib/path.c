#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "hook/hr.h"
#include "hook/table.h"

#include "hooklib/path.h"

/* Helpers */

static void path_hook_init(void);
static BOOL path_transform_a(char **out, const char *src);
static BOOL path_transform_w(wchar_t **out, const wchar_t *src);

/* API hooks */

static BOOL WINAPI hook_CreateDirectoryA(
        const char *lpFileName,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static BOOL WINAPI hook_CreateDirectoryW(
        const wchar_t *lpFileName,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static BOOL WINAPI hook_CreateDirectoryExA(
        const char *lpTemplateDirectory,
        const char *lpNewDirectory,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static BOOL WINAPI hook_CreateDirectoryExW(
        const wchar_t *lpTemplateDirectory,
        const wchar_t *lpNewDirectory,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static HANDLE WINAPI hook_CreateFileA(
        const char *lpFileName,
        uint32_t dwDesiredAccess,
        uint32_t dwShareMode,
        SECURITY_ATTRIBUTES *lpSecurityAttributes,
        uint32_t dwCreationDisposition,
        uint32_t dwFlagsAndAttributes,
        HANDLE hTemplateFile);

static HANDLE WINAPI hook_CreateFileW(
        const wchar_t *lpFileName,
        uint32_t dwDesiredAccess,
        uint32_t dwShareMode,
        SECURITY_ATTRIBUTES *lpSecurityAttributes,
        uint32_t dwCreationDisposition,
        uint32_t dwFlagsAndAttributes,
        HANDLE hTemplateFile);

static HANDLE WINAPI hook_FindFirstFileA(
        const char *lpFileName,
        LPWIN32_FIND_DATAA lpFindFileData);

static HANDLE WINAPI hook_FindFirstFileW(
        const wchar_t *lpFileName,
        LPWIN32_FIND_DATAW lpFindFileData);

static HANDLE WINAPI hook_FindFirstFileExA(
        const char *lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        void *lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        void *lpSearchFilter,
        DWORD dwAdditionalFlags);

static HANDLE WINAPI hook_FindFirstFileExW(
        const wchar_t *lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        void *lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        void *lpSearchFilter,
        DWORD dwAdditionalFlags);

static DWORD WINAPI hook_GetFileAttributesA(const char *lpFileName);

static DWORD WINAPI hook_GetFileAttributesW(const wchar_t *lpFileName);

static BOOL WINAPI hook_GetFileAttributesExA(
        const char *lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        void *lpFileInformation);

static BOOL WINAPI hook_GetFileAttributesExW(
        const wchar_t *lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        void *lpFileInformation);

static BOOL WINAPI hook_RemoveDirectoryA(const char *lpFileName);

static BOOL WINAPI hook_RemoveDirectoryW(const wchar_t *lpFileName);

/* Link pointers */

static BOOL (WINAPI *next_CreateDirectoryA)(
        const char *lpFileName,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static BOOL (WINAPI *next_CreateDirectoryW)(
        const wchar_t *lpFileName,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static BOOL (WINAPI *next_CreateDirectoryExA)(
        const char *lpTemplateDirectory,
        const char *lpNewDirectory,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static BOOL (WINAPI *next_CreateDirectoryExW)(
        const wchar_t *lpTemplateDirectory,
        const wchar_t *lpNewDirectory,
        SECURITY_ATTRIBUTES *lpSecurityAttributes);

static HANDLE (WINAPI *next_CreateFileA)(
        const char *lpFileName,
        uint32_t dwDesiredAccess,
        uint32_t dwShareMode,
        SECURITY_ATTRIBUTES *lpSecurityAttributes,
        uint32_t dwCreationDisposition,
        uint32_t dwFlagsAndAttributes,
        HANDLE hTemplateFile);

static HANDLE (WINAPI *next_CreateFileW)(
        const wchar_t *lpFileName,
        uint32_t dwDesiredAccess,
        uint32_t dwShareMode,
        SECURITY_ATTRIBUTES *lpSecurityAttributes,
        uint32_t dwCreationDisposition,
        uint32_t dwFlagsAndAttributes,
        HANDLE hTemplateFile);

static HANDLE (WINAPI *next_FindFirstFileA)(
        const char *lpFileName,
        LPWIN32_FIND_DATAA lpFindFileData);

static HANDLE (WINAPI *next_FindFirstFileW)(
        const wchar_t *lpFileName,
        LPWIN32_FIND_DATAW lpFindFileData);

static HANDLE (WINAPI *next_FindFirstFileExA)(
        const char *lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        void *lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        void *lpSearchFilter,
        DWORD dwAdditionalFlags);

static HANDLE (WINAPI *next_FindFirstFileExW)(
        const wchar_t *lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        void *lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        void *lpSearchFilter,
        DWORD dwAdditionalFlags);

static DWORD (WINAPI *next_GetFileAttributesA)(const char *lpFileName);

static DWORD (WINAPI *next_GetFileAttributesW)(const wchar_t *lpFileName);

static BOOL (WINAPI *next_GetFileAttributesExA)(
        const char *lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        void *lpFileInformation);

static BOOL (WINAPI *next_GetFileAttributesExW)(
        const wchar_t *lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        void *lpFileInformation);

static BOOL (WINAPI *next_RemoveDirectoryA)(const char *lpFileName);

static BOOL (WINAPI *next_RemoveDirectoryW)(const wchar_t *lpFileName);

/* Hook table */

static const struct hook_symbol path_hook_syms[] = {
    {
        .name   = "CreateDirectoryA",
        .patch  = hook_CreateDirectoryA,
        .link   = (void **) &next_CreateDirectoryA,
    }, {
        .name   = "CreateDirectoryW",
        .patch  = hook_CreateDirectoryW,
        .link   = (void **) &next_CreateDirectoryW,
    }, {
        .name   = "CreateDirectoryExA",
        .patch  = hook_CreateDirectoryExA,
        .link   = (void **) &next_CreateDirectoryExA,
    }, {
        .name   = "CreateDirectoryExW",
        .patch  = hook_CreateDirectoryExW,
        .link   = (void **) &next_CreateDirectoryExW,
    }, {
        .name   = "CreateFileA",
        .patch  = hook_CreateFileA,
        .link   = (void **) &next_CreateFileA,
    }, {
        .name   = "CreateFileW",
        .patch  = hook_CreateFileW,
        .link   = (void **) &next_CreateFileW,
    }, {
        .name   = "FindFirstFileA",
        .patch  = hook_FindFirstFileA,
        .link   = (void **) &next_FindFirstFileA,
    }, {
        .name   = "FindFirstFileW",
        .patch  = hook_FindFirstFileW,
        .link   = (void **) &next_FindFirstFileW,
    }, {
        .name   = "FindFirstFileExA",
        .patch  = hook_FindFirstFileExA,
        .link   = (void **) &next_FindFirstFileExA,
    }, {
        .name   = "FindFirstFileExW",
        .patch  = hook_FindFirstFileExW,
        .link   = (void **) &next_FindFirstFileExW,
    }, {
        .name   = "GetFileAttributesA",
        .patch  = hook_GetFileAttributesA,
        .link   = (void **) &next_GetFileAttributesA,
    }, {
        .name   = "GetFileAttributesW",
        .patch  = hook_GetFileAttributesW,
        .link   = (void **) &next_GetFileAttributesW,
    }, {
        .name   = "GetFileAttributesExA",
        .patch  = hook_GetFileAttributesExA,
        .link   = (void **) &next_GetFileAttributesExA,
    }, {
        .name   = "GetFileAttributesExW",
        .patch  = hook_GetFileAttributesExW,
        .link   = (void **) &next_GetFileAttributesExW,
    }, {
        .name   = "RemoveDirectoryA",
        .patch  = hook_RemoveDirectoryA,
        .link   = (void **) &next_RemoveDirectoryA,
    }, {
        .name   = "RemoveDirectoryW",
        .patch  = hook_RemoveDirectoryW,
        .link   = (void **) &next_RemoveDirectoryW,
    }
};

static bool path_hook_initted;
static CRITICAL_SECTION path_hook_lock;
static path_hook_t *path_hook_list;
static size_t path_hook_count;

HRESULT path_hook_push(path_hook_t hook)
{
    path_hook_t *tmp;
    HRESULT hr;

    assert(hook != NULL);

    path_hook_init();

    EnterCriticalSection(&path_hook_lock);

    tmp = realloc(
            path_hook_list,
            (path_hook_count + 1) * sizeof(path_hook_t));

    if (tmp == NULL) {
        hr = E_OUTOFMEMORY;

        goto end;
    }

    path_hook_list = tmp;
    path_hook_list[path_hook_count++] = hook;

    hr = S_OK;

end:
    LeaveCriticalSection(&path_hook_lock);

    return hr;
}

static void path_hook_init(void)
{
    /* Init is not thread safe because API hook init is not thread safe blah
       blah blah you know the drill by now. */

    if (path_hook_initted) {
        return;
    }

    path_hook_initted = true;
    InitializeCriticalSection(&path_hook_lock);

    path_hook_insert_hooks(NULL);
}

void path_hook_insert_hooks(HMODULE target)
{
    hook_table_apply(
            target,
            "kernel32.dll",
            path_hook_syms,
            _countof(path_hook_syms));
}

static BOOL path_transform_a(char **out, const char *src)
{
    wchar_t *src_w;
    size_t src_c;
    wchar_t *dest_w;
    char *dest_a;
    size_t dest_s;
    BOOL ok;

    assert(out != NULL);

    src_w = NULL;
    dest_w = NULL;
    dest_a = NULL;
    *out = NULL;

    if (src == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        ok = FALSE;

        goto end;
    }

    /* Widen the path */

    mbstowcs_s(&src_c, NULL, 0, src, 0);
    src_w = malloc(src_c * sizeof(wchar_t));

    if (src_w == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        ok = FALSE;

        goto end;
    }

    mbstowcs_s(NULL, src_w, src_c, src, src_c - 1);

    /* Try applying a path transform */

    ok = path_transform_w(&dest_w, src_w); /* Take ownership! */

    if (!ok || dest_w == NULL) {
        goto end;
    }

    /* Narrow the transformed path */

    wcstombs_s(&dest_s, NULL, 0, dest_w, 0);
    dest_a = malloc(dest_s * sizeof(char));

    if (dest_a == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        ok = FALSE;

        goto end;
    }

    wcstombs_s(NULL, dest_a, dest_s, dest_w, dest_s - 1);

    *out = dest_a; /* Relinquish ownership to caller! */
    ok = TRUE;

end:
    free(dest_w);
    free(src_w);

    return ok;
}

static BOOL path_transform_w(wchar_t **out, const wchar_t *src)
{
    BOOL ok;
    HRESULT hr;
    wchar_t *dest;
    size_t dest_c;
    size_t i;

    assert(out != NULL);

    dest = NULL;
    *out = NULL;

    EnterCriticalSection(&path_hook_lock);

    for (i = 0 ; i < path_hook_count ; i++) {
        hr = path_hook_list[i](src, NULL, &dest_c);

        if (FAILED(hr)) {
            ok = hr_propagate_win32(hr, FALSE);

            goto end;
        }

        if (hr == S_FALSE) {
            continue;
        }

        dest = malloc(dest_c * sizeof(wchar_t));

        if (dest == NULL) {
            SetLastError(ERROR_OUTOFMEMORY);
            ok = FALSE;

            goto end;
        }

        hr = path_hook_list[i](src, dest, &dest_c);

        if (FAILED(hr)) {
            ok = hr_propagate_win32(hr, FALSE);

            goto end;
        }

        break;
    }

    *out = dest;
    dest = NULL;
    ok = TRUE;

end:
    LeaveCriticalSection(&path_hook_lock);

    free(dest);

    return ok;
}

int path_compare_w(const wchar_t *string1, const wchar_t *string2, size_t count)
{
    size_t i;
    wchar_t c1, c2;

    assert(string1 != NULL);
    assert(string2 != NULL);

    for (i = 0; i < count && string1[i] && string2[i]; i++) {
        c1 = towlower(string1[i]);

        if (c1 == '/') {
            c1 = '\\';
        }

        c2 = towlower(string2[i]);

        if (c2 == '/') {
            c2 = '\\';
        }

        if (c1 != c2) {
            break;
        }
    }

    return i == count ? 0 : string2[i] - string1[i];
}

/* Dumping ground for kernel32 file system ops whose path parameters we have to
   hook into and translate. This list will grow over time as we go back and
   fix up older games that don't pay attention to the mount point registry. */

static BOOL WINAPI hook_CreateDirectoryA(
        const char *lpFileName,
        SECURITY_ATTRIBUTES *lpSecurityAttributes)
{
    char *trans;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return FALSE;
    }

    ok = next_CreateDirectoryA(
            trans ? trans : lpFileName,
            lpSecurityAttributes);

    free(trans);

    return ok;
}

static BOOL WINAPI hook_CreateDirectoryW(
        const wchar_t *lpFileName,
        SECURITY_ATTRIBUTES *lpSecurityAttributes)
{
    wchar_t *trans;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return FALSE;
    }

    ok = next_CreateDirectoryW(
            trans ? trans : lpFileName,
            lpSecurityAttributes);

    free(trans);

    return ok;
}

static BOOL WINAPI hook_CreateDirectoryExA(
        const char *lpTemplateDirectory,
        const char *lpNewDirectory,
        SECURITY_ATTRIBUTES *lpSecurityAttributes)
{
    char *trans;
    BOOL ok;

    ok = path_transform_a(&trans, lpNewDirectory);

    if (!ok) {
        return FALSE;
    }

    ok = next_CreateDirectoryExA(
            lpTemplateDirectory,
            trans ? trans : lpNewDirectory,
            lpSecurityAttributes);

    free(trans);

    return ok;
}

static BOOL WINAPI hook_CreateDirectoryExW(
        const wchar_t *lpTemplateDirectory,
        const wchar_t *lpNewDirectory,
        SECURITY_ATTRIBUTES *lpSecurityAttributes)
{
    wchar_t *trans;
    BOOL ok;

    ok = path_transform_w(&trans, lpNewDirectory);

    if (!ok) {
        return FALSE;
    }

    ok = next_CreateDirectoryExW(
            lpTemplateDirectory,
            trans ? trans : lpNewDirectory,
            lpSecurityAttributes);

    free(trans);

    return ok;
}

/* Don't pull in the entire iohook framework just for CreateFileA/CreateFileW */

static HANDLE WINAPI hook_CreateFileA(
        const char *lpFileName,
        uint32_t dwDesiredAccess,
        uint32_t dwShareMode,
        SECURITY_ATTRIBUTES *lpSecurityAttributes,
        uint32_t dwCreationDisposition,
        uint32_t dwFlagsAndAttributes,
        HANDLE hTemplateFile)
{
    char *trans;
    HANDLE result;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return INVALID_HANDLE_VALUE;
    }

    result = next_CreateFileA(
            trans ? trans : lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);

    free(trans);

    return result;
}

static HANDLE WINAPI hook_CreateFileW(
        const wchar_t *lpFileName,
        uint32_t dwDesiredAccess,
        uint32_t dwShareMode,
        SECURITY_ATTRIBUTES *lpSecurityAttributes,
        uint32_t dwCreationDisposition,
        uint32_t dwFlagsAndAttributes,
        HANDLE hTemplateFile)
{
    wchar_t *trans;
    HANDLE result;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return INVALID_HANDLE_VALUE;
    }

    result = next_CreateFileW(
            trans ? trans : lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile);

    free(trans);

    return result;
}

static HANDLE WINAPI hook_FindFirstFileA(
        const char *lpFileName,
        LPWIN32_FIND_DATAA lpFindFileData)
{
    char *trans;
    HANDLE result;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return INVALID_HANDLE_VALUE;
    }

    result = next_FindFirstFileA(trans ? trans : lpFileName, lpFindFileData);

    free(trans);

    return result;
}

static HANDLE WINAPI hook_FindFirstFileW(
        const wchar_t *lpFileName,
        LPWIN32_FIND_DATAW lpFindFileData)
{
    wchar_t *trans;
    HANDLE result;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return INVALID_HANDLE_VALUE;
    }

    result = next_FindFirstFileW(trans ? trans : lpFileName, lpFindFileData);

    free(trans);

    return result;
}

static HANDLE WINAPI hook_FindFirstFileExA(
        const char *lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        void *lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        void *lpSearchFilter,
        DWORD dwAdditionalFlags)
{
    char *trans;
    HANDLE result;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return INVALID_HANDLE_VALUE;
    }

    result = next_FindFirstFileExA(
            trans ? trans : lpFileName,
            fInfoLevelId,
            lpFindFileData,
            fSearchOp,
            lpSearchFilter,
            dwAdditionalFlags);

    free(trans);

    return result;
}

static HANDLE WINAPI hook_FindFirstFileExW(
        const wchar_t *lpFileName,
        FINDEX_INFO_LEVELS fInfoLevelId,
        void *lpFindFileData,
        FINDEX_SEARCH_OPS fSearchOp,
        void *lpSearchFilter,
        DWORD dwAdditionalFlags)
{
    wchar_t *trans;
    HANDLE result;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return INVALID_HANDLE_VALUE;
    }

    result = next_FindFirstFileExW(
            trans ? trans : lpFileName,
            fInfoLevelId,
            lpFindFileData,
            fSearchOp,
            lpSearchFilter,
            dwAdditionalFlags);

    free(trans);

    return result;
}

static DWORD WINAPI hook_GetFileAttributesA(const char *lpFileName)
{
    char *trans;
    DWORD result;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return INVALID_FILE_ATTRIBUTES;
    }

    result = next_GetFileAttributesA(trans ? trans : lpFileName);
    free(trans);

    return result;
}

static DWORD WINAPI hook_GetFileAttributesW(const wchar_t *lpFileName)
{
    wchar_t *trans;
    DWORD result;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return INVALID_FILE_ATTRIBUTES;
    }

    result = next_GetFileAttributesW(trans ? trans : lpFileName);

    free(trans);

    return result;
}

static BOOL WINAPI hook_GetFileAttributesExA(
        const char *lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        void *lpFileInformation)
{
    char *trans;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return INVALID_FILE_ATTRIBUTES;
    }

    ok = next_GetFileAttributesExA(
            trans ? trans : lpFileName,
            fInfoLevelId,
            lpFileInformation);

    free(trans);

    return ok;
}

static BOOL WINAPI hook_GetFileAttributesExW(
        const wchar_t *lpFileName,
        GET_FILEEX_INFO_LEVELS fInfoLevelId,
        void *lpFileInformation)
{
    wchar_t *trans;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return INVALID_FILE_ATTRIBUTES;
    }

    ok = next_GetFileAttributesExW(
            trans ? trans : lpFileName,
            fInfoLevelId,
            lpFileInformation);

    free(trans);

    return ok;
}

static BOOL WINAPI hook_RemoveDirectoryA(const char *lpFileName)
{
    char *trans;
    BOOL ok;

    ok = path_transform_a(&trans, lpFileName);

    if (!ok) {
        return FALSE;
    }

    ok = next_RemoveDirectoryA(trans ? trans : lpFileName);

    free(trans);

    return ok;
}

static BOOL WINAPI hook_RemoveDirectoryW(const wchar_t *lpFileName)
{
    wchar_t *trans;
    BOOL ok;

    ok = path_transform_w(&trans, lpFileName);

    if (!ok) {
        return FALSE;
    }

    ok = next_RemoveDirectoryW(trans ? trans : lpFileName);

    free(trans);

    return ok;
}
