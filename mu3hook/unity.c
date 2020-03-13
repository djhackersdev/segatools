#include <stdbool.h>

#include <windows.h>

#include "hook/table.h"

#include "hooklib/dll.h"
#include "hooklib/path.h"

#include "util/dprintf.h"

static void dll_hook_insert_hooks(HMODULE target);

static HMODULE WINAPI my_LoadLibraryW(const wchar_t *name);
static HMODULE (WINAPI *next_LoadLibraryW)(const wchar_t *name);

static const struct hook_symbol unity_kernel32_syms[] = {
    {
        .name = "LoadLibraryW",
        .patch = my_LoadLibraryW,
        .link = (void **) &next_LoadLibraryW,
    },
};

static const wchar_t *target_modules[] = {
    L"mono.dll",
    L"cri_ware_unity.dll",
};
static const size_t target_modules_len = _countof(target_modules);

void unity_hook_init(void)
{
    dll_hook_insert_hooks(NULL);
}

static void dll_hook_insert_hooks(HMODULE target)
{
    hook_table_apply(
            target,
            "kernel32.dll",
            unity_kernel32_syms,
            _countof(unity_kernel32_syms));
}

static HMODULE WINAPI my_LoadLibraryW(const wchar_t *name)
{
    const wchar_t *name_end;
    const wchar_t *target_module;
    bool already_loaded;
    HMODULE result;
    size_t name_len;
    size_t target_module_len;

    if (name == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);

        return NULL;
    }

    // Check if the module is already loaded
    already_loaded = GetModuleHandleW(name) != NULL;

    // Must call the next handler so the DLL reference count is incremented
    result = next_LoadLibraryW(name);

    if (!already_loaded && result != NULL) {
        name_len = wcslen(name);

        for (size_t i = 0; i < target_modules_len; i++) {
            target_module = target_modules[i];
            target_module_len = wcslen(target_module);

            // Check if the newly loaded library is at least the length of
            // the name of the target module
            if (name_len < target_module_len) {
                continue;
            }

            name_end = &name[name_len - target_module_len];

            // Check if the name of the newly loaded library is one of the
            // modules the path hooks should be injected into
            if (_wcsicmp(name_end, target_module) != 0) {
                continue;
            }

            dprintf("Unity: Loaded %S\n", target_module);

            dll_hook_insert_hooks(result);
            path_hook_insert_hooks(result);
        }
    }

    return result;
}
