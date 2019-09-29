#include <windows.h>
#include <dinput.h>

#include <assert.h>

#include "idzio/di-dev.h"

#include "util/dprintf.h"

HRESULT idz_di_dev_start(IDirectInputDevice8W *dev, HWND wnd)
{
    HRESULT hr;

    assert(dev != NULL);
    assert(wnd != NULL);

    hr = IDirectInputDevice8_SetCooperativeLevel(
            dev,
            wnd,
            DISCL_BACKGROUND | DISCL_EXCLUSIVE);

    if (FAILED(hr)) {
        dprintf("DirectInput: SetCooperativeLevel failed: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInputDevice8_SetDataFormat(dev, &c_dfDIJoystick);

    if (FAILED(hr)) {
        dprintf("DirectInput: SetDataFormat failed: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInputDevice8_Acquire(dev);

    if (FAILED(hr)) {
        dprintf("DirectInput: Acquire failed: %08x\n", (int) hr);

        return hr;
    }

    return hr;
}

void idz_di_dev_start_fx(IDirectInputDevice8W *dev, IDirectInputEffect **out)
{
    /* Set up force-feedback on devices that support it. This is just a stub
       for the time being, since we don't yet know how the serial port force
       feedback protocol works.

       I'm currently developing with an Xbox One Thrustmaster TMX wheel, if
       we don't perform at least some perfunctory FFB initialization of this
       nature (or indeed if no DirectInput application is running) then the
       wheel exhibits considerable resistance, similar to that of a stationary
       car. Changing cf.lMagnitude to a nonzero value does cause the wheel to
       continuously turn in the given direction with the given force as one
       would expect (max magnitude per DirectInput docs is +/- 10000).

       Failure here is non-fatal, we log any errors and move on.

       https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee416353(v=vs.85)
    */

    IDirectInputEffect *obj;
    DWORD axis;
    LONG direction;
    DIEFFECT fx;
    DICONSTANTFORCE cf;
    HRESULT hr;

    assert(dev != NULL);
    assert(out != NULL);

    *out = NULL;

    dprintf("DirectInput: Starting force feedback (may take a sec)\n");

    axis = DIJOFS_X;
    direction = 0;

    memset(&cf, 0, sizeof(cf));
    cf.lMagnitude = 0;

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    fx.dwDuration = INFINITE;
    fx.dwGain = DI_FFNOMINALMAX;
    fx.dwTriggerButton = DIEB_NOTRIGGER;
    fx.dwTriggerRepeatInterval = INFINITE;
    fx.cAxes = 1;
    fx.rgdwAxes = &axis;
    fx.rglDirection = &direction;
    fx.cbTypeSpecificParams = sizeof(cf);
    fx.lpvTypeSpecificParams = &cf;

    hr = IDirectInputDevice8_CreateEffect(
            dev,
            &GUID_ConstantForce,
            &fx,
            &obj,
            NULL);

    if (FAILED(hr)) {
        dprintf("DirectInput: DirectInput force feedback unavailable: %08x\n",
                (int) hr);

        return;
    }

    hr = IDirectInputEffect_Start(obj, INFINITE, 0);

    if (FAILED(hr)) {
        IDirectInputEffect_Release(obj);
        dprintf("DirectInput: DirectInput force feedback start failed: %08x\n",
                (int) hr);

        return;
    }

    *out = obj;

    dprintf("DirectInput: Force feedback initialized and set to zero\n");
}

HRESULT idz_di_dev_poll(
        IDirectInputDevice8W *dev,
        HWND wnd,
        union idz_di_state *out)
{
    HRESULT hr;
    MSG msg;

    assert(dev != NULL);
    assert(wnd != NULL);
    assert(out != NULL);

    memset(out, 0, sizeof(*out));

    /* Pump our dummy window's message queue just in case DirectInput or an
       IHV DirectInput driver somehow relies on it */

    while (PeekMessageW(&msg, wnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hr = IDirectInputDevice8_GetDeviceState(
            dev,
            sizeof(out->st),
            &out->st);

    if (FAILED(hr)) {
        dprintf("DirectInput: GetDeviceState error: %08x\n", (int) hr);
    }

    /* JVS lacks a protocol for reporting hardware errors from poll command
       responses, so this ends up returning zeroed input state instead. */

    return hr;
}
