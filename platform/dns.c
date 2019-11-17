#include <windows.h>

#include <assert.h>

#include "hooklib/dns.h"

#include "platform/dns.h"

HRESULT dns_platform_hook_init(const struct dns_config *cfg)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    hr = dns_hook_push(L"tenporouter.loc", cfg->router);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"bbrouter.loc", cfg->router);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"naominet.jp", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"anbzvarg.wc", cfg->startup);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"ib.naominet.jp", cfg->billing);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"vo.anbzvarg.wc", cfg->billing);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"aime.naominet.jp", cfg->aimedb);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"nvzr.anbzvarg.wc", cfg->aimedb);

    if (FAILED(hr)) {
        return hr;
    }

    // if your ISP resolves bad domains, it will kill the network. These 2
    // *cannot* resolve

    hr = dns_hook_push(L"mobirouter.loc", NULL);

    if (FAILED(hr)) {
        return hr;
    }

    hr = dns_hook_push(L"dslrouter.loc", NULL);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}
