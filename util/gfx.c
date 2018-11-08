#include <windows.h>
#include <d3d9.h>

#include <stdlib.h>

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "util/dprintf.h"

static HRESULT STDMETHODCALLTYPE my_CreateDevice(
        IDirect3D9 *self,
        UINT adapter,
        D3DDEVTYPE type,
        HWND hwnd,
        DWORD flags,
        D3DPRESENT_PARAMETERS *pp,
        IDirect3DDevice9 **pdev);

static IDirect3D9 * WINAPI my_Direct3DCreate9(UINT sdk_ver);

static IDirect3D9 * (WINAPI *next_Direct3DCreate9)(UINT sdk_ver);

static bool gfx_windowed;

static const struct hook_symbol gfx_hooks[] = {
    {
        .name   = "Direct3DCreate9",
        .patch  = my_Direct3DCreate9,
        .link   = (void **) &next_Direct3DCreate9
    },
};

void gfx_hook_init(void)
{
    hook_table_apply(NULL, "d3d9.dll", gfx_hooks, _countof(gfx_hooks));
}

void gfx_set_windowed(void)
{
    gfx_windowed = true;
}

static IDirect3D9 * WINAPI my_Direct3DCreate9(UINT sdk_ver)
{
    struct com_proxy *proxy;
    IDirect3D9Vtbl *vtbl;
    IDirect3D9 *api;
    HRESULT hr;

    dprintf("Direct3DCreate9 hook hit\n");

    api = next_Direct3DCreate9(sdk_ver);

    if (api == NULL) {
        dprintf("next_Direct3DCreate9 returned NULL\n");

        goto fail;
    }

    hr = com_proxy_wrap(&proxy, api, sizeof(*api->lpVtbl));

    if (FAILED(hr)) {
        dprintf("com_proxy_wrap returned %x\n", (int) hr);

        goto fail;
    }

    vtbl = proxy->vptr;
    vtbl->CreateDevice = my_CreateDevice;

    return (IDirect3D9 *) proxy;

fail:
    if (api != NULL) {
        IDirect3D9_Release(api);
    }

    return NULL;
}

static HRESULT STDMETHODCALLTYPE my_CreateDevice(
        IDirect3D9 *self,
        UINT adapter,
        D3DDEVTYPE type,
        HWND hwnd,
        DWORD flags,
        D3DPRESENT_PARAMETERS *pp,
        IDirect3DDevice9 **pdev)
{
    struct com_proxy *proxy;
    IDirect3D9 *real;

    dprintf("IDirect3D9::CreateDevice hook hit\n");

    proxy = com_proxy_downcast(self);
    real = proxy->real;

    if (gfx_windowed) {
        pp->Windowed = TRUE;
        pp->FullScreen_RefreshRateInHz = 0;
    }

    return IDirect3D9_CreateDevice(real, adapter, type, hwnd, flags, pp, pdev);
}
