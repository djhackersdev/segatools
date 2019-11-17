/* Might push this to capnhook, don't add any util dependencies. */

#include <windows.h>
#include <windns.h>
#include <ws2tcpip.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hook/hr.h"
#include "hook/table.h"

#include "hooklib/dns.h"

/* Latest w32headers does not include DnsQueryEx, so we'll have to "polyfill"
   its associated data types here for the time being.

   Results and cancel handle are passed through, so we'll just use void
   pointers for those args. So are most of the fields in this structure, for
   that matter. */

typedef struct POLYFILL_DNS_QUERY_REQUEST {
    ULONG Version;
    PCWSTR QueryName;
    WORD QueryType;
    ULONG64 QueryOptions;
    void* pDnsServerList;
    ULONG InterfaceIndex;
    void* pQueryCompletionCallback;
    PVOID pQueryContext;
} POLYFILL_DNS_QUERY_REQUEST;

struct dns_hook_entry {
    wchar_t *from;
    wchar_t *to;
};

/* Hook funcs */

static DNS_STATUS WINAPI hook_DnsQuery_A(
        const char *pszName,
        WORD wType,
        DWORD Options,
        void *pExtra,
        DNS_RECORD **ppQueryResults,
        void *pReserved);

static DNS_STATUS WINAPI hook_DnsQuery_W(
        const wchar_t *pszName,
        WORD wType,
        DWORD Options,
        void *pExtra,
        DNS_RECORD **ppQueryResults,
        void *pReserved);

static DNS_STATUS WINAPI hook_DnsQueryEx(
        POLYFILL_DNS_QUERY_REQUEST *pRequest,
        void *pQueryResults,
        void *pCancelHandle);

static int WSAAPI hook_getaddrinfo(
        const char *pNodeName,
        const char *pServiceName,
        const ADDRINFOA *pHints,
        ADDRINFOA **ppResult);

/* Link pointers */

static DNS_STATUS (WINAPI *next_DnsQuery_A)(
        const char *pszName,
        WORD wType,
        DWORD Options,
        void *pExtra,
        DNS_RECORD **ppQueryResults,
        void *pReserved);

static DNS_STATUS (WINAPI *next_DnsQuery_W)(
        const wchar_t *pszName,
        WORD wType,
        DWORD Options,
        void *pExtra,
        DNS_RECORD **ppQueryResults,
        void *pReserved);

static DNS_STATUS (WINAPI *next_DnsQueryEx)(
        POLYFILL_DNS_QUERY_REQUEST *pRequest,
        void *pQueryResults,
        void *pCancelHandle);

static int (WSAAPI *next_getaddrinfo)(
        const char *pNodeName,
        const char *pServiceName,
        const ADDRINFOA *pHints,
        ADDRINFOA **ppResult);

static const struct hook_symbol dns_hook_syms_dnsapi[] = {
    {
        .name       = "DnsQuery_A",
        .patch      = hook_DnsQuery_A,
        .link       = (void **) &next_DnsQuery_A,
    }, {
        .name       = "DnsQuery_W",
        .patch      = hook_DnsQuery_W,
        .link       = (void **) &next_DnsQuery_W,
    }, {
        .name       = "DnsQueryEx",
        .patch      = hook_DnsQueryEx,
        .link       = (void **) &next_DnsQueryEx,
    }
};

static const struct hook_symbol dns_hook_syms_ws2[] = {
    {
        .name       = "getaddrinfo",
        .ordinal    = 176,
        .patch      = hook_getaddrinfo,
        .link       = (void **) &next_getaddrinfo,
    }
};

static bool dns_hook_initted;
static CRITICAL_SECTION dns_hook_lock;
static struct dns_hook_entry *dns_hook_entries;
static size_t dns_hook_nentries;

static void dns_hook_init(void)
{
    if (dns_hook_initted) {
        return;
    }

    dns_hook_initted = true;
    InitializeCriticalSection(&dns_hook_lock);

    hook_table_apply(
            NULL,
            "dnsapi.dll",
            dns_hook_syms_dnsapi,
            _countof(dns_hook_syms_dnsapi));

    hook_table_apply(
            NULL,
            "ws2_32.dll",
            dns_hook_syms_ws2,
            _countof(dns_hook_syms_ws2));
}

HRESULT dns_hook_push(const wchar_t *from_src, const wchar_t *to_src)
{
    HRESULT hr;
    struct dns_hook_entry *newmem;
    struct dns_hook_entry *newitem;
    wchar_t *from;
    wchar_t *to;

    assert(from_src != NULL);

    to = NULL;
    from = NULL;
    dns_hook_init();

    EnterCriticalSection(&dns_hook_lock);

    from = _wcsdup(from_src);

    if (from == NULL) {
        hr = E_OUTOFMEMORY;

        goto end;
    }

    if(to_src != NULL) {
        to = _wcsdup(to_src);

        if (to == NULL) {
            hr = E_OUTOFMEMORY;

            goto end;
        }
    }

    newmem = realloc(
            dns_hook_entries,
            (dns_hook_nentries + 1) * sizeof(struct dns_hook_entry));

    if (newmem == NULL) {
        hr = E_OUTOFMEMORY;

        goto end;
    }

    dns_hook_entries = newmem;
    newitem = &newmem[dns_hook_nentries++];
    newitem->from = from;
    newitem->to = to;

    from = NULL;
    to = NULL;
    hr = S_OK;

end:
    LeaveCriticalSection(&dns_hook_lock);

    free(to);
    free(from);

    return hr;
}

static DNS_STATUS WINAPI hook_DnsQuery_A(
        const char *pszName,
        WORD wType,
        DWORD Options,
        void *pExtra,
        DNS_RECORD **ppQueryResults,
        void *pReserved)
{
    const struct dns_hook_entry *pos;
    size_t i;
    size_t wstr_c;
    wchar_t *wstr;
    size_t str_c;
    char *str;
    DNS_STATUS code;
    HRESULT hr;

    wstr = NULL;
    str = NULL;

    if (pszName == NULL) {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

        goto end;
    }

    mbstowcs_s(&wstr_c, NULL, 0, pszName, 0);
    wstr = malloc(wstr_c * sizeof(wchar_t));

    if (wstr == NULL) {
        hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

        goto end;
    }

    mbstowcs_s(NULL, wstr, wstr_c, pszName, wstr_c - 1);
    EnterCriticalSection(&dns_hook_lock);

    for (i = 0 ; i < dns_hook_nentries ; i++) {
        pos = &dns_hook_entries[i];

        if (_wcsicmp(wstr, pos->from) == 0) {
            if(pos->to == NULL) {
                LeaveCriticalSection(&dns_hook_lock);
                hr = HRESULT_FROM_WIN32(DNS_ERROR_RCODE_NAME_ERROR);

                goto end;
            }

            wcstombs_s(&str_c, NULL, 0, pos->to, 0);
            str = malloc(str_c * sizeof(char));

            if (str == NULL) {
                LeaveCriticalSection(&dns_hook_lock);
                hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

                goto end;
            }

            wcstombs_s(NULL, str, str_c, pos->to, str_c - 1);
            pszName = str;

            break;
        }
    }

    LeaveCriticalSection(&dns_hook_lock);

    code = next_DnsQuery_A(
            pszName,
            wType,
            Options,
            pExtra,
            ppQueryResults,
            pReserved);

    hr = HRESULT_FROM_WIN32(code);

end:
    free(str);
    free(wstr);

    return hr_to_win32_error(hr);
}

static DNS_STATUS WINAPI hook_DnsQuery_W(
        const wchar_t *pszName,
        WORD wType,
        DWORD Options,
        void *pExtra,
        DNS_RECORD **ppQueryResults,
        void *pReserved)
{
    const struct dns_hook_entry *pos;
    size_t i;

    if (pszName == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&dns_hook_lock);

    for (i = 0 ; i < dns_hook_nentries ; i++) {
        pos = &dns_hook_entries[i];

        if (_wcsicmp(pszName, pos->from) == 0) {
            if(pos->to == NULL) {
                LeaveCriticalSection(&dns_hook_lock);
                return HRESULT_FROM_WIN32(DNS_ERROR_RCODE_NAME_ERROR);
            }

            pszName = pos->to;

            break;
        }
    }

    LeaveCriticalSection(&dns_hook_lock);

    return next_DnsQuery_W(
            pszName,
            wType,
            Options,
            pExtra,
            ppQueryResults,
            pReserved);

}

static DNS_STATUS WINAPI hook_DnsQueryEx(
        POLYFILL_DNS_QUERY_REQUEST *pRequest,
        void *pQueryResults,
        void *pCancelHandle)
{
    const wchar_t *orig;
    const struct dns_hook_entry *pos;
    DNS_STATUS code;
    size_t i;

    if (pRequest == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    orig = pRequest->QueryName;
    EnterCriticalSection(&dns_hook_lock);

    for (i = 0 ; i < dns_hook_nentries ; i++) {
        pos = &dns_hook_entries[i];

        if (_wcsicmp(pRequest->QueryName, pos->from) == 0) {
            if(pos->to == NULL) {
                LeaveCriticalSection(&dns_hook_lock);
                return HRESULT_FROM_WIN32(DNS_ERROR_RCODE_NAME_ERROR);
            }

            pRequest->QueryName = pos->to;

            break;
        }
    }

    LeaveCriticalSection(&dns_hook_lock);

    code = next_DnsQueryEx(pRequest, pQueryResults, pCancelHandle);

    /* Caller might not appreciate QueryName changing under its feet. It is
       strongly implied by MSDN that a copy of *pRequest is taken by WINAPI,
       so we can change it back after the call has been issued with no ill
       effect... we hope.

       Hopefully the completion callback is issued from an APC or something
       (or otherwise happens after this returns) or we're in trouble. */

    pRequest->QueryName = orig;

    return code;
}

static int WSAAPI hook_getaddrinfo(
        const char *pNodeName,
        const char *pServiceName,
        const ADDRINFOA *pHints,
        ADDRINFOA **ppResult)
{
    const struct dns_hook_entry *pos;
    char *str;
    size_t str_c;
    wchar_t *wstr;
    size_t wstr_c;
    int result;
    size_t i;

    str = NULL;
    wstr = NULL;

    if (pNodeName == NULL) {
        result = WSA_INVALID_PARAMETER;

        goto end;
    }

    mbstowcs_s(&wstr_c, NULL, 0, pNodeName, 0);
    wstr = malloc(wstr_c * sizeof(wchar_t));

    if (wstr == NULL) {
        result = WSA_NOT_ENOUGH_MEMORY;

        goto end;
    }

    mbstowcs_s(NULL, wstr, wstr_c, pNodeName, wstr_c - 1);
    EnterCriticalSection(&dns_hook_lock);

    for (i = 0 ; i < dns_hook_nentries ; i++) {
        pos = &dns_hook_entries[i];

        if (_wcsicmp(wstr, pos->from) == 0) {
            if(pos->to == NULL) {
                LeaveCriticalSection(&dns_hook_lock);
                result = EAI_NONAME;

                goto end;
            }

            wcstombs_s(&str_c, NULL, 0, pos->to, 0);
            str = malloc(str_c * sizeof(char));

            if (str == NULL) {
                LeaveCriticalSection(&dns_hook_lock);
                result = WSA_NOT_ENOUGH_MEMORY;

                goto end;
            }

            wcstombs_s(NULL, str, str_c, pos->to, str_c - 1);
            pNodeName = str;

            break;
        }
    }

    LeaveCriticalSection(&dns_hook_lock);

    result = next_getaddrinfo(pNodeName, pServiceName, pHints, ppResult);

end:
    free(wstr);
    free(str);

    return result;
}
