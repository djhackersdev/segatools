#include <windows.h>

#include <assert.h>
#include <string.h>

#include "util/dprintf.h"

/* DirectInput requires a window for correct initialization (and also force
   feedback), so this source file provides some utilities for creating a
   generic message-only window. */

static LRESULT WINAPI idz_io_wnd_proc(
            HWND hwnd,
            UINT msg,
            WPARAM wparam,
            LPARAM lparam);

HRESULT idz_io_wnd_create(HINSTANCE inst, HWND *out)
{
    HRESULT hr;
    WNDCLASSEXW wcx;
    ATOM atom;
    HWND hwnd;

    assert(inst != NULL); /* We are not an EXE */
    assert(out != NULL);

    *out = NULL;

    memset(&wcx, 0, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = idz_io_wnd_proc;
    wcx.hInstance = inst;
    wcx.lpszClassName = L"IDZIO";

    atom = RegisterClassExW(&wcx);

    if (atom == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("IDZIO: RegisterClassExW failed: %08x\n", (int) hr);

        goto fail;
    }

    hwnd = CreateWindowExW(
            0,
            (wchar_t *) (intptr_t) atom,
            L"",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            HWND_MESSAGE,
            NULL,
            inst,
            NULL);

    if (hwnd == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("IDZIO: CreateWindowExW failed: %08x\n", (int) hr);

        goto fail;
    }

    *out = hwnd;

    return S_OK;

fail:
    UnregisterClassW((wchar_t *) (intptr_t) atom, inst);

    return hr;
}

static LRESULT WINAPI idz_io_wnd_proc(
            HWND hwnd,
            UINT msg,
            WPARAM wparam,
            LPARAM lparam)
{
    switch (msg) {
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}
