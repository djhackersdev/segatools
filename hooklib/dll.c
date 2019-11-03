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
    const struct hook_symbol *syms;
    size_t nsyms;
};

/* Helper functions */

static void dll_hook_init(void);
static HMODULE dll_hook_search_dll(const wchar_t *name);

/* Hook functions */

static HMODULE WINAPI hook_GetModuleHandleA(const char *name);
static HMODULE WINAPI hook_GetModuleHandleW(const wchar_t *name);
static HMODULE WINAPI hook_LoadLibraryA(const char *name);
static HMODULE WINAPI hook_LoadLibraryW(const wchar_t *name);
static void * WINAPI hook_GetProcAddress(HMODULE mod, const char *name);

/* Link pointers */

static HMODULE (WINAPI *next_GetModuleHandleA)(const char *name);
static HMODULE (WINAPI *next_GetModuleHandleW)(const wchar_t *name);
static HMODULE (WINAPI *next_LoadLibraryA)(const char *name);
static HMODULE (WINAPI *next_LoadLibraryW)(const wchar_t *name);
static void * (WINAPI *next_GetProcAddress)(HMODULE mod, const char *name);

static const struct hook_symbol dll_loader_syms[] = {
    {
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
        .name   = "GetProcAddress",
        .patch  = hook_GetProcAddress,
        .link   = (void **) &next_GetProcAddress,
    }
};

static bool dll_hook_initted;
static CRITICAL_SECTION dll_hook_lock;
static struct dll_hook_reg *dll_hook_list;
static size_t dll_hook_count;

HRESULT dll_hook_push(
        HMODULE redir_mod,
        const wchar_t *name,
        const struct hook_symbol *syms,
        size_t nsyms)
{
    struct dll_hook_reg *new_item;
    struct dll_hook_reg *new_mem;
    HRESULT hr;

    assert(name != NULL);
    assert(syms != NULL);

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
    new_item->syms = syms;
    new_item->nsyms = nsyms;

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
       process imports LoadLibraryW but something imports LoadLibraryA.

       We know something imports GetModuleHandleW because we do, right here.

       (we're about to hook these APIs of course, so we have to set this up
       before the hooks go in) */

    kernel32 = GetModuleHandleW(L"kernel32.dll");
    next_LoadLibraryW = (void *) GetProcAddress(kernel32, "LoadLibraryW");

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

/* TODO LoadLibraryExA, LoadLibraryExW */

static void * WINAPI hook_GetProcAddress(HMODULE mod, const char *name)
{
    const struct hook_symbol *syms;
    uintptr_t ordinal;
    size_t nsyms;
    size_t i;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    syms = NULL;
    nsyms = 0;

    EnterCriticalSection(&dll_hook_lock);

    for (i = 0 ; i < dll_hook_count ; i++) {
        if (dll_hook_list[i].redir_mod == mod) {
            syms = dll_hook_list[i].syms;
            nsyms = dll_hook_list[i].nsyms;

            break;
        }
    }

    LeaveCriticalSection(&dll_hook_lock);

    if (syms == NULL) {
        return next_GetProcAddress(mod, name);
    }

    ordinal = (uintptr_t) name;

    if (ordinal > 0xFFFF) {
        /* Import by name */

        for (i = 0 ; i < nsyms ; i++) {
            if (strcmp(name, syms[i].name) == 0) {
                break;
            }
        }
    } else {
        /* Import by ordinal (and name != NULL so ordinal != 0) */

        for (i = 0 ; i < nsyms ; i++) {
            if (ordinal == syms[i].ordinal) {
                break;
            }
        }
    }

    if (i < nsyms) {
        SetLastError(ERROR_SUCCESS);

        return syms[i].patch;
    } else {
        /* GetProcAddress sets this error on failure, although of course MSDN
           does not see fit to document the exact error code. */
        SetLastError(ERROR_PROC_NOT_FOUND);

        return NULL;
    }
}
