#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/table.h"

#include "hooklib/reg.h"

#include "util/dprintf.h"
#include "util/str.h"

struct reg_hook_key {
    HKEY root;
    const wchar_t *name;
    const struct reg_hook_val *vals;
    size_t nvals;
    HKEY handle;
};

/* Helper functions */

static void reg_hook_init(void);

static LRESULT reg_hook_propagate_hr(HRESULT hr);

static struct reg_hook_key *reg_hook_match_key_locked(HKEY handle);

static const struct reg_hook_val *reg_hook_match_val_locked(
        struct reg_hook_key *key,
        const wchar_t *name);

static LSTATUS reg_hook_open_locked(
        HKEY parent,
        const wchar_t *name,
        HKEY *out);

static LSTATUS reg_hook_query_val_locked(
        struct reg_hook_key *key,
        const wchar_t *name,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes);

/* API hooks */

static LSTATUS WINAPI hook_RegOpenKeyExW(
        HKEY parent,
        const wchar_t *name,
        uint32_t flags,
        uint32_t access,
        HKEY *out);

static LSTATUS WINAPI hook_RegCreateKeyExW(
        HKEY parent,
        const wchar_t *name,
        uint32_t reserved,
        const wchar_t *class_,
        uint32_t options,
        uint32_t access,
        const SECURITY_ATTRIBUTES *sa,
        HKEY *out,
        uint32_t *disposition);

static LSTATUS WINAPI hook_RegCloseKey(HKEY handle);

static LSTATUS WINAPI hook_RegQueryValueExA(
        HKEY handle,
        const char *name,
        void *reserved,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes);

static LSTATUS WINAPI hook_RegQueryValueExW(
        HKEY handle,
        const wchar_t *name,
        void *reserved,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes);

static LSTATUS WINAPI hook_RegSetValueExW(
        HKEY handle,
        const wchar_t *name,
        uint32_t reserved,
        uint32_t type,
        const void *bytes,
        uint32_t nbytes);

/* Link pointers */

static LSTATUS WINAPI (*next_RegOpenKeyExW)(
        HKEY parent,
        const wchar_t *name,
        uint32_t flags,
        uint32_t access,
        HKEY *out);

static LSTATUS WINAPI (*next_RegCreateKeyExW)(
        HKEY parent,
        const wchar_t *name,
        uint32_t reserved,
        const wchar_t *class_,
        uint32_t options,
        uint32_t access,
        const SECURITY_ATTRIBUTES *sa,
        HKEY *out,
        uint32_t *disposition);

static LSTATUS WINAPI (*next_RegCloseKey)(HKEY handle);

static LSTATUS WINAPI (*next_RegQueryValueExA)(
        HKEY handle,
        const char *name,
        void *reserved,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes);

static LSTATUS WINAPI (*next_RegQueryValueExW)(
        HKEY handle,
        const wchar_t *name,
        void *reserved,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes);

static LSTATUS WINAPI (*next_RegSetValueExW)(
        HKEY handle,
        const wchar_t *name,
        uint32_t reserved,
        uint32_t type,
        const void *bytes,
        uint32_t nbytes);

static const struct hook_symbol reg_hook_syms[] = {
    {
        .name   = "RegOpenKeyExW",
        .patch  = hook_RegOpenKeyExW,
        .link   = (void **) &next_RegOpenKeyExW,
    }, {
        .name   = "RegCreateKeyExW",
        .patch  = hook_RegCreateKeyExW,
        .link   = (void **) &next_RegCreateKeyExW,
    }, {
        .name   = "RegCloseKey",
        .patch  = hook_RegCloseKey,
        .link   = (void **) &next_RegCloseKey,
    }, {
        .name   = "RegQueryValueExA",
        .patch  = hook_RegQueryValueExA,
        .link   = (void **) &next_RegQueryValueExA,
    }, {
        .name   = "RegQueryValueExW",
        .patch  = hook_RegQueryValueExW,
        .link   = (void **) &next_RegQueryValueExW,
    }, {
        .name   = "RegSetValueExW",
        .patch  = hook_RegSetValueExW,
        .link   = (void **) &next_RegSetValueExW,
    }
};

static bool reg_hook_initted;
static CRITICAL_SECTION reg_hook_lock;
static struct reg_hook_key *reg_hook_keys;
static size_t reg_hook_nkeys;

HRESULT reg_hook_push_key(
        HKEY root,
        const wchar_t *name,
        const struct reg_hook_val *vals,
        size_t nvals)
{
    struct reg_hook_key *new_mem;
    struct reg_hook_key *new_key;
    HRESULT hr;

    assert(root != NULL);
    assert(name != NULL);
    assert(vals != NULL || nvals == 0);

    reg_hook_init();

    EnterCriticalSection(&reg_hook_lock);

    new_mem = realloc(
            reg_hook_keys,
            (reg_hook_nkeys + 1) * sizeof(struct reg_hook_key));

    if (new_mem == NULL) {
        hr = E_OUTOFMEMORY;

        goto end;
    }

    new_key = &new_mem[reg_hook_nkeys];
    memset(new_key, 0, sizeof(*new_key));
    new_key->root = root;
    new_key->name = name; /* Expect this to be statically allocated */
    new_key->vals = vals;
    new_key->nvals = nvals;

    reg_hook_keys = new_mem;
    reg_hook_nkeys++;

    hr = S_OK;

end:
    LeaveCriticalSection(&reg_hook_lock);

    return hr;
}

static void reg_hook_init(void)
{
    if (reg_hook_initted) {
        return;
    }

    reg_hook_initted = true;
    InitializeCriticalSection(&reg_hook_lock);

    hook_table_apply(
            NULL,
            "advapi32.dll",
            reg_hook_syms,
            _countof(reg_hook_syms));
}

static LRESULT reg_hook_propagate_hr(HRESULT hr)
{
    if (SUCCEEDED(hr)) {
        return ERROR_SUCCESS;
    } else if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
        return HRESULT_CODE(hr);
    } else {
        return ERROR_GEN_FAILURE;
    }
}

static struct reg_hook_key *reg_hook_match_key_locked(HKEY handle)
{
    struct reg_hook_key *key;
    size_t i;

    if (handle == NULL || handle == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    for (i = 0 ; i < reg_hook_nkeys ; i++) {
        key = &reg_hook_keys[i];

        if (key->handle == handle) {
            return key;
        }
    }

    return NULL;
}

static const struct reg_hook_val *reg_hook_match_val_locked(
        struct reg_hook_key *key,
        const wchar_t *name)
{
    const struct reg_hook_val *val;
    size_t i;

    /* Watch out for accesses to the key's default value */

    if (name == NULL) {
        name = L"";
    }

    for (i = 0 ; i < key->nvals ; i++) {
        val = &key->vals[i];

        if (wstr_ieq(val->name, name)) {
            return val;
        }
    }

    return NULL;
}

static LSTATUS reg_hook_open_locked(
        HKEY parent,
        const wchar_t *name,
        HKEY *out)
{
    struct reg_hook_key *key;
    LSTATUS err;
    size_t i;

    *out = NULL;

    for (i = 0 ; i < reg_hook_nkeys ; i++) {
        /* Assume reg keys are referenced from a root key and not from some
           intermediary key */
        key = &reg_hook_keys[i];

        if (key->root == parent && wstr_ieq(key->name, name)) {
            break;
        }
    }

    /* (Bail out if we didn't find anything; this causes the open/create call
       to be passed onward down the hook chain) */

    if (i >= reg_hook_nkeys) {
        return ERROR_SUCCESS;
    }

    /* Assume only one handle will be open at a time */

    if (key->handle != NULL) {
        return ERROR_SHARING_VIOLATION;
    }

    /* Open a unique HKEY handle that we can use to identify accesses to
       this virtual registry key. We open a read-only handle to an arbitrary
       registry key that we can reliably assume exists and isn't one of the
       hardcoded root handles. HKLM\SOFTWARE will suffice for this purpose. */

    err = next_RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE",
            0,
            KEY_READ,
            out);

    if (err == ERROR_SUCCESS) {
        key->handle = *out;
    }

    return err;
}

static LSTATUS WINAPI hook_RegOpenKeyExW(
        HKEY parent,
        const wchar_t *name,
        uint32_t flags,
        uint32_t access,
        HKEY *out)
{
    LSTATUS err;

    if (out == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&reg_hook_lock);
    err = reg_hook_open_locked(parent, name, out);
    LeaveCriticalSection(&reg_hook_lock);

    if (err == ERROR_SUCCESS) {
        if (*out != NULL) {
            //dprintf("Registry: Opened virtual key %S\n", name);
        } else {
            err = next_RegOpenKeyExW(parent, name, flags, access, out);
        }
    }

    return err;
}

static LSTATUS WINAPI hook_RegCreateKeyExW(
        HKEY parent,
        const wchar_t *name,
        uint32_t reserved,
        const wchar_t *class_,
        uint32_t options,
        uint32_t access,
        const SECURITY_ATTRIBUTES *sa,
        HKEY *out,
        uint32_t *disposition)
{
    LSTATUS err;

    if (out == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&reg_hook_lock);
    err = reg_hook_open_locked(parent, name, out);
    LeaveCriticalSection(&reg_hook_lock);

    if (err == ERROR_SUCCESS) {
        if (*out != NULL) {
            //dprintf("Registry: Created virtual key %S\n", name);
        } else {
            err = next_RegCreateKeyExW(
                    parent,
                    name,
                    reserved,
                    class_,
                    options,
                    access,
                    sa,
                    out,
                    disposition);
        }
    }

    return err;
}

static LSTATUS WINAPI hook_RegCloseKey(HKEY handle)
{
    struct reg_hook_key *key;
    size_t i;

    EnterCriticalSection(&reg_hook_lock);

    for (i = 0 ; i < reg_hook_nkeys ; i++) {
        key = &reg_hook_keys[i];

        if (key->handle == handle) {
            //dprintf("Registry: Closed virtual key %S\n", key->name);
            key->handle = NULL;
        }
    }

    LeaveCriticalSection(&reg_hook_lock);

    return next_RegCloseKey(handle);
}

static LSTATUS WINAPI hook_RegQueryValueExW(
        HKEY handle,
        const wchar_t *name,
        void *reserved,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes)
{
    struct reg_hook_key *key;
    LSTATUS err;

    EnterCriticalSection(&reg_hook_lock);

    key = reg_hook_match_key_locked(handle);

    /* Check if this is a virtualized registry key */

    if (key == NULL) {
        LeaveCriticalSection(&reg_hook_lock);

        return next_RegQueryValueExW(
                handle,
                name,
                reserved,
                type,
                bytes,
                nbytes);
    }

    /* Call the factored out core of this function because RegQueryValueExA
       has to be a blight upon my existence */

    err = reg_hook_query_val_locked(key, name, type, bytes, nbytes);

    LeaveCriticalSection(&reg_hook_lock);

    return err;
}

/* now this right here is a pain in my ass */

static LSTATUS WINAPI hook_RegQueryValueExA(
        HKEY handle,
        const char *name,
        void *reserved,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes)
{
    /* _s: sizeof, _c: _countof(), _w: widened */

    struct reg_hook_key *key;
    wchar_t *name_w;
    size_t name_c;
    wchar_t *content;
    uint32_t content_s;
    size_t content_c;
    uint32_t type_site;
    LSTATUS err;

    name_w = NULL;
    content = NULL;

    /* Normalize inconvenient inputs */

    if (name == NULL) {
        name = "";
    }

    if (type == NULL) {
        type = &type_site;
    }

    /* Look up key handle, early exit if no match */

    EnterCriticalSection(&reg_hook_lock);
    key = reg_hook_match_key_locked(handle);

    if (key == NULL) {
        LeaveCriticalSection(&reg_hook_lock);

        return next_RegQueryValueExA(
                handle,
                name,
                reserved,
                type,
                bytes,
                nbytes);
    }

    /* OK, first off we need to widen the name. This requires a temporary
       buffer allocation. */

    mbstowcs_s(&name_c, NULL, 0, name, 0);
    name_w = malloc(name_c * sizeof(wchar_t));

    if (name_w == NULL) {
        err = ERROR_OUTOFMEMORY;

        goto end;
    }

    mbstowcs_s(NULL, name_w, name_c, name, name_c - 1);

    /* Next, check to see if the caller even cares about the content. We can
       pass through if they don't. */

    if (bytes == NULL && nbytes == NULL) {
        err = reg_hook_query_val_locked(key, name_w, type, NULL, NULL);

        goto end;
    }

    /* Next, we need to check the key type to see if it's REG_SZ. */

    err = reg_hook_query_val_locked(key, name_w, type, NULL, NULL);

    if (err != ERROR_SUCCESS) {
        goto end;
    }

    /* If it is not REG_SZ then pass the content directly.
       (We ignore the REG_MULTI_SZ case here). */

    assert(*type != REG_MULTI_SZ);

    if (*type != REG_SZ) {
        err = reg_hook_query_val_locked(key, name_w, type, bytes, nbytes);

        goto end;
    }

    /* Otherwise things get more complicated. First we must measure the wide-
       character length of the value (hopefully said value does not change
       under our feet, of course). */

    err = reg_hook_query_val_locked(key, name_w, type, NULL, &content_s);

    if (err != ERROR_SUCCESS) {
        goto end;
    }

    /* Next, allocate a scratch buffer. Even if the caller doesn't supply an
       output buffer we need to know the actual content to be able to size the
       narrow version. */

    content = malloc(content_s);

    if (content == NULL) {
        err = ERROR_OUTOFMEMORY;

        goto end;
    }

    /* Get the data... */

    err = reg_hook_query_val_locked(key, name_w, type, content, &content_s);

    if (err != ERROR_SUCCESS) {
        goto end;
    }

    /* Now size the corresponding narrow form and return it to the caller */

    wcstombs_s(&content_c, NULL, 0, content, 0);

    if (bytes != NULL) {
        if (nbytes == NULL) {
            err = ERROR_INVALID_PARAMETER;

            goto end;
        }

        if (*nbytes < content_c) {
            err = ERROR_MORE_DATA;

            goto end;
        }

        wcstombs_s(NULL, bytes, *nbytes, content, content_c - 1);
    }

    if (nbytes != NULL) { /* It really should be, based on earlier checks ... */
        *nbytes = content_c;
    }

    err = ERROR_SUCCESS;

end:
    LeaveCriticalSection(&reg_hook_lock);

    free(content);
    free(name_w);

    return err;
}

static LSTATUS reg_hook_query_val_locked(
        struct reg_hook_key *key,
        const wchar_t *name,
        uint32_t *type,
        void *bytes,
        uint32_t *nbytes)
{
    const struct reg_hook_val *val;
    LSTATUS err;
    HRESULT hr;

    val = reg_hook_match_val_locked(key, name);

    if (val != NULL) {
        if (type != NULL) {
            *type = val->type;
        }

        if (val->read != NULL) {
            hr = val->read(bytes, nbytes);
            err = reg_hook_propagate_hr(hr);
        } else {
            dprintf("Registry: %S: Val %S has no read handler\n",
                    key->name,
                    name);

            err = ERROR_ACCESS_DENIED;
        }
    } else {
        dprintf("Registry: Key %S: Val %S not found\n", key->name, name);
        err = ERROR_FILE_NOT_FOUND;
    }

    return err;
}

static LSTATUS WINAPI hook_RegSetValueExW(
        HKEY handle,
        const wchar_t *name,
        uint32_t reserved,
        uint32_t type,
        const void *bytes,
        uint32_t nbytes)
{
    struct reg_hook_key *key;
    const struct reg_hook_val *val;
    LSTATUS err;
    HRESULT hr;

    EnterCriticalSection(&reg_hook_lock);

    key = reg_hook_match_key_locked(handle);

    if (key == NULL) {
        LeaveCriticalSection(&reg_hook_lock);

        return next_RegSetValueExW(
                handle,
                name,
                reserved,
                type,
                bytes,
                nbytes);
    }

    val = reg_hook_match_val_locked(key, name);

    if (val != NULL) {
        if (val->write != NULL) {
            if (type != val->type) {
                dprintf(        "Registry: Key %S: Val %S: Type mismatch "
                                "(expected %i got %i)\n",
                        key->name,
                        name,
                        val->type,
                        type);

                err = ERROR_ACCESS_DENIED;
            } else {
                dprintf("Registry: Write virtual key %S value %S\n",
                        key->name,
                        val->name);

                hr = val->write(bytes, nbytes);
                err = reg_hook_propagate_hr(hr);
            }
        } else {
            /* No write handler (the common case), black-hole whatever gets
               written. */

            err = ERROR_SUCCESS;
        }
    } else {
        dprintf("Registry: Key %S: Val %S not found\n", key->name, name);
        err = ERROR_FILE_NOT_FOUND;
    }

    LeaveCriticalSection(&reg_hook_lock);

    return err;
}

HRESULT reg_hook_read_bin(
        void *bytes,
        uint32_t *nbytes,
        const void *src_bytes,
        size_t src_nbytes)
{
    assert(src_bytes != NULL || src_nbytes == 0);

    if (bytes != NULL) {
        if (nbytes == NULL || *nbytes < src_nbytes) {
            return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }

        memcpy(bytes, src_bytes, src_nbytes);
    }

    if (nbytes != NULL) {
        *nbytes = src_nbytes;
    }

    return S_OK;
}

HRESULT reg_hook_read_u32(
        void *bytes,
        uint32_t *nbytes,
        uint32_t src)
{
    if (bytes != NULL) {
        if (nbytes == NULL || *nbytes < sizeof(uint32_t)) {
            return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }

        memcpy(bytes, &src, sizeof(uint32_t));
    }

    if (nbytes != NULL) {
        *nbytes = sizeof(uint32_t);
    }

    return S_OK;
}

HRESULT reg_hook_read_wstr(
        void *bytes,
        uint32_t *nbytes,
        const wchar_t *src)
{
    size_t src_nbytes;

    assert(src != NULL);

    src_nbytes = (wcslen(src) + 1) * sizeof(wchar_t);

    if (bytes != NULL) {
        if (nbytes == NULL || *nbytes < src_nbytes) {
            return HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }

        memcpy(bytes, src, src_nbytes);
    }

    if (nbytes != NULL) {
        *nbytes = src_nbytes;
    }

    return S_OK;
}
