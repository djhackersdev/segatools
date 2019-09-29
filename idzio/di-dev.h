#pragma once

#include <windows.h>
#include <dinput.h>

#include <stdint.h>

union idz_di_state {
    DIJOYSTATE st;
    uint8_t bytes[sizeof(DIJOYSTATE)];
};

HRESULT idz_di_dev_start(IDirectInputDevice8W *dev, HWND wnd);
void idz_di_dev_start_fx(IDirectInputDevice8W *dev, IDirectInputEffect **out);
HRESULT idz_di_dev_poll(
        IDirectInputDevice8W *dev,
        HWND wnd,
        union idz_di_state *out);

