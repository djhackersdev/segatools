/* This is general enough to break out into capnhook eventually.
   Don't introduce util/ dependencies here. */

#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "hook/table.h"

#include "hooklib/dll.h"

struct dll_hook_reg {
    const wchar_t *name;
    HMODULE redir_mod;
};

/* Helper functions */

static void dll_hook_init(void);
static HMODULE dll_hook_search_dll(const wchar_t *name);

/* Hook functions */

static BOOL WINAPI hook_FreeLibrary(HMODULE mod);
static HMODULE WINAPI hook_GetModuleHandleA(const char *name);
static HMODULE WINAPI hook_GetModuleHandleW(const wchar_t *name);
static HMODULE WINAPI hook_LoadLibraryA(const char *name);
static HMODULE WINAPI hook_LoadLibraryW(const wchar_t *name);
static HMODULE WINAPI hook_LoadLibraryExA(const char *name, HANDLE file, DWORD flags);
static HMODULE WINAPI hook_LoadLibraryExW(const wchar_t *name, HANDLE file, DWORD flags);

/* Link pointers */

static BOOL (WINAPI *next_FreeLibrary)(HMODULE mod);
static HMODULE (WINAPI *next_GetModuleHandleA)(const char *name);
static HMODULE (WINAPI *next_GetModuleHandleW)(const wchar_t *name);
static HMODULE (WINAPI *next_LoadLibraryA)(const char *name);
static HMODULE (WINAPI *next_LoadLibraryW)(const wchar_t *name);
static HMODULE (WINAPI *next_LoadLibraryExA)(const char *name, HANDLE file, DWORD flags);
static HMODULE (WINAPI *next_LoadLibraryExW)(const wchar_t *name, HANDLE file, DWORD flags);

static const struct hook_symbol dll_loader_syms[] = {
    {
        .name   = "FreeLibrary",
        .patch  = hook_FreeLibrary,
        .link   = (void **) &next_FreeLibrary,
    }, {
        .name   = "GetModuleHandleA",
        .patch  = hook_GetModuleHandleA,
        .link   = (void **) &next_GetModuleHandleA,
    }, {
        .name   = "GetModuleHandleW",
        .patch  = hook_GetModuleHandleW,
        .link   = (void **) &next_GetModuleHandleW,
    }, {
        .name   = "LoadLibraryA",
        .patch  = hook_LoadLibraryA,
        .link   = (void **) &next_LoadLibraryA,
    }, {
        .name   = "LoadLibraryW",
        .patch  = hook_LoadLibraryW,
        .link   = (void **) &next_LoadLibraryW,
    }, {
        .name   = "LoadLibraryExA",
        .patch  = hook_LoadLibraryExA,
        .link   = (void **) &next_LoadLibraryExA,
    }, {
        .name   = "LoadLibraryExW",
        .patch  = hook_LoadLibraryExW,
        .link   = (void **) &next_LoadLibraryExW,
    }
};

static bool dll_hook_initted;
static CRITICAL_SECTION dll_hook_lock;
static struct dll_hook_reg *dll_hook_list;
static size_t dll_hook_count;

HRESULT dll_hook_push(
        HMODULE redir_mod,
        const wchar_t *name)
{
    struct dll_hook_reg *new_item;
    struct dll_hook_reg *new_mem;
    HRESULT hr;

    assert(name != NULL);

    dll_hook_init();

    EnterCriticalSection(&dll_hook_lock);

    new_mem = realloc(
            dll_hook_list,
            (dll_hook_count + 1) * sizeof(struct dll_hook_reg));

    if (new_mem == NULL) {
        hr = E_OUTOFMEMORY;

        goto end;
    }

    new_item = &new_mem[dll_hook_count];
    new_item->name = name;
    new_item->redir_mod = redir_mod;

    dll_hook_list = new_mem;
    dll_hook_count++;
    hr = S_OK;

end:
    LeaveCriticalSection(&dll_hook_lock);

    return hr;
}

static void dll_hook_init(void)
{
    HMODULE kernel32;

    /* Init is not thread safe, because API hooking is not thread safe. */

    if (dll_hook_initted) {
        return;
    }

    dll_hook_initted = true;
    InitializeCriticalSection(&dll_hook_lock);

    /* Protect against the (probably impossible) scenario where nothing in the
       process imports LoadLibraryW but something imports LoadLibraryA. Also
       do the same with LoadLibraryExW.

       We know something imports GetModuleHandleW because we do, right here.

       (we're about to hook these APIs of course, so we have to set this up
       before the hooks go in) */

    kernel32 = GetModuleHandleW(L"kernel32.dll");
    next_LoadLibraryW = (void *) GetProcAddress(kernel32, "LoadLibraryW");
    next_LoadLibraryExW = (void *) GetProcAddress(kernel32, "LoadLibraryExW");

    /* Now we can apply the hook table */

    hook_table_apply(
            NULL,
            "kernel32.dll",
            dll_loader_syms,
            _countof(dll_loader_syms));
}

static HMODULE dll_hook_search_dll(const wchar_t *name)
{
    HMODULE result;
    size_t i;

    result = NULL;

    EnterCriticalSection(&dll_hook_lock);

    for (i = 0 ; i < dll_hook_count ; i++) {
        if (wcsicmp(name, dll_hook_list[i].name) == 0) {
            result = dll_hook_list[i].redir_mod;

            break;
        }
    }

    LeaveCriticalSection(&dll_hook_lock);

    return result;
}

static BOOL WINAPI hook_FreeLibrary(HMODULE mod)
{
    bool match;
    size_t i;

    match = false;
    EnterCriticalSection(&dll_hook_lock);

    for (i = 0 ; i < dll_hook_count ; i++) {
        if (mod == dll_hook_list[i].redir_mod) {
            match = true;

            break;
        }
    }

    LeaveCriticalSection(&dll_hook_lock);

    if (match) {
        /* Block attempts to unload redirected modules, since this could cause
           a hook DLL to unexpectedly vanish and crash the whole application.

           Reference counting might be another solution, although it is
           possible that a buggy application might cause a hook DLL unload in
           that case. */
        SetLastError(ERROR_SUCCESS);

        return TRUE;
    }

    return next_FreeLibrary(mod);
}

static HMODULE WINAPI hook_GetModuleHandleA(const char *name)
{
    HMODULE result;
    wchar_t *name_w;
    size_t name_c;

    if (name == NULL) {
        return next_GetModuleHandleA(NULL);
    }

    mbstowcs_s(&name_c, NULL, 0, name, 0);
    name_w = malloc(name_c * sizeof(wchar_t));

    if (name_w == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);

        return NULL;
    }

    mbstowcs_s(NULL, name_w, name_c, name, name_c - 1);
    result = hook_GetModuleHandleW(name_w);
    free(name_w);

    return result;
}

static HMODULE WINAPI hook_GetModuleHandleW(const wchar_t *name)
{
    HMODULE result;

    if (name == NULL) {
        return next_GetModuleHandleW(NULL);
    }

    result = dll_hook_search_dll(name);

    if (result != NULL) {
        SetLastError(ERROR_SUCCESS);
    } else {
        result = next_GetModuleHandleW(name);
    }

    return result;
}

static HMODULE WINAPI hook_LoadLibraryA(const char *name)
{
    HMODULE result;
    wchar_t *name_w;
    size_t name_c;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    mbstowcs_s(&name_c, NULL, 0, name, 0);
    name_w = malloc(name_c * sizeof(wchar_t));

    if (name_w == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);

        return NULL;
    }

    mbstowcs_s(NULL, name_w, name_c, name, name_c - 1);
    result = hook_LoadLibraryW(name_w);
    free(name_w);

    return result;
}

static HMODULE WINAPI hook_LoadLibraryW(const wchar_t *name)
{
    HMODULE result;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    result = dll_hook_search_dll(name);

    if (result != NULL) {
        SetLastError(ERROR_SUCCESS);
    } else {
        result = next_LoadLibraryW(name);
    }

    return result;
}

static HMODULE WINAPI hook_LoadLibraryExA(const char *name, HANDLE file, DWORD flags)
{
    HMODULE result;
    wchar_t *name_w;
    size_t name_c;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    mbstowcs_s(&name_c, NULL, 0, name, 0);
    name_w = malloc(name_c * sizeof(wchar_t));

    if (name_w == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);

        return NULL;
    }

    mbstowcs_s(NULL, name_w, name_c, name, name_c - 1);
    result = hook_LoadLibraryExW(name_w, file, flags);
    free(name_w);

    return result;
}

static HMODULE WINAPI hook_LoadLibraryExW(const wchar_t *name, HANDLE file, DWORD flags)
{
    HMODULE result;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    result = dll_hook_search_dll(name);

    if (result != NULL) {
        SetLastError(ERROR_SUCCESS);
    } else {
        result = next_LoadLibraryExW(name, file, flags);
    }

    return result;
}
