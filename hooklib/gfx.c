#include <windows.h>
#include <d3d9.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/com-proxy.h"
#include "hook/table.h"

#include "hooklib/config.h"
#include "hooklib/gfx.h"

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
static HRESULT gfx_frame_window(HWND hwnd);

static struct gfx_config gfx_config;

static const struct hook_symbol gfx_hooks[] = {
    {
        .name   = "Direct3DCreate9",
        .patch  = my_Direct3DCreate9,
        .link   = (void **) &next_Direct3DCreate9
    },
};

void gfx_hook_init(const struct gfx_config *cfg)
{
    assert(cfg != NULL);

    if (!cfg->enable) {
        return;
    }

    memcpy(&gfx_config, cfg, sizeof(*cfg));
    hook_table_apply(NULL, "d3d9.dll", gfx_hooks, _countof(gfx_hooks));
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

    if (gfx_config.windowed) {
        pp->Windowed = TRUE;
        pp->FullScreen_RefreshRateInHz = 0;
    }

    if (gfx_config.framed) {
        gfx_frame_window(hwnd);
    }

    return IDirect3D9_CreateDevice(real, adapter, type, hwnd, flags, pp, pdev);
}

static HRESULT gfx_frame_window(HWND hwnd)
{
    HRESULT hr;
    DWORD error;
    LONG style;
    RECT rect;
    BOOL ok;

    SetLastError(ERROR_SUCCESS);
    style = GetWindowLongW(hwnd, GWL_STYLE);
    error = GetLastError();

    if (error != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32(error);
        dprintf("Gfx: GetWindowLongPtrW(%p, GWL_STYLE) failed: %x\n",
                hwnd,
                (int) hr);

        return hr;
    }

    ok = GetClientRect(hwnd, &rect);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: GetClientRect(%p) failed: %x\n", hwnd, (int) hr);

        return hr;
    }

    style |= WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
    ok = AdjustWindowRect(&rect, style, FALSE);

    if (!ok) {
        /* come on... */
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: AdjustWindowRect failed: %x\n", (int) hr);

        return hr;
    }

    /* This... always seems to set an error, even though it works? idk */
    SetWindowLongW(hwnd, GWL_STYLE, style);

    ok = SetWindowPos(
            hwnd,
            HWND_TOP,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED | SWP_NOMOVE);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Gfx: SetWindowPos(%p) failed: %x\n", hwnd, (int) hr);

        return hr;
    }

    return S_OK;
}
