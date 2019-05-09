#include <windows.h>
#include <dinput.h>

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "idzio/backend.h"
#include "idzio/idzio.h"
#include "idzio/shifter.h"
#include "idzio/wnd.h"

#include "util/dprintf.h"

static BOOL idz_di_enum_callback(const DIDEVICEINSTANCEW *dev, void *ctx);
static void idz_di_try_fx(void);
static HRESULT idz_di_poll(DIJOYSTATE *out);
static void idz_di_jvs_read_buttons(uint8_t *gamebtn_out);
static uint8_t idz_di_decode_pov(DWORD pov);
static void idz_di_jvs_read_shifter(uint8_t *gear);
static void idz_di_jvs_read_analogs(struct idz_io_analog_state *out);

static const struct idz_io_backend idz_di_backend = {
    .jvs_read_buttons   = idz_di_jvs_read_buttons,
    .jvs_read_shifter   = idz_di_jvs_read_shifter,
    .jvs_read_analogs   = idz_di_jvs_read_analogs,
};

static HWND idz_di_wnd;
static IDirectInput8W *idz_di_api;
static IDirectInputDevice8W *idz_di_dev;
static IDirectInputEffect *idz_di_fx;

HRESULT idz_di_init(HINSTANCE inst, const struct idz_io_backend **backend)
{
    HRESULT hr;
    HMODULE dinput8;
    HRESULT WINAPI (*api_entry)(HINSTANCE,DWORD,REFIID,LPVOID *,LPUNKNOWN);
    wchar_t dll_path[MAX_PATH];
    UINT path_pos;

    assert(backend != NULL);

    *backend = NULL;

    hr = idz_io_wnd_create(inst, &idz_di_wnd);

    if (FAILED(hr)) {
        return hr;
    }

    /* Initial D Zero has some built-in DirectInput support that is not
       particularly useful. We short this out by dropping a no-op dinput8.dll
       into the install directory. However, this DLL does need to talk to the
       real operating system implementation of DirectInput without the stub DLL
       interfering, so build a path to C:\Windows\System32\dinput.dll here. */

    dll_path[0] = L'\0';
    path_pos = GetSystemDirectoryW(dll_path, _countof(dll_path));
    wcscat_s(
            dll_path + path_pos,
            _countof(dll_path) - path_pos,
            L"\\dinput8.dll");

    dinput8 = LoadLibraryW(dll_path);

    if (dinput8 == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Wheel: LoadLibrary failed: %08x\n", (int) hr);

        return hr;
    }

    api_entry = (void *) GetProcAddress(dinput8, "DirectInput8Create");

    if (api_entry == NULL) {
        dprintf("Wheel: GetProcAddress failed\n");

        return E_FAIL;
    }

    hr = api_entry(
            inst,
            DIRECTINPUT_VERSION,
            &IID_IDirectInput8W,
            (void **) &idz_di_api,
            NULL);

    if (FAILED(hr)) {
        dprintf("Wheel: API create failed: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInput8_EnumDevices(
            idz_di_api,
            DI8DEVCLASS_GAMECTRL,
            idz_di_enum_callback,
            NULL,
            DIEDFL_ATTACHEDONLY);

    if (FAILED(hr) || idz_di_dev == NULL) {
        dprintf("Wheel: Could not find a controller: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInputDevice8_SetCooperativeLevel(
            idz_di_dev,
            idz_di_wnd,
            DISCL_BACKGROUND | DISCL_EXCLUSIVE);

    if (FAILED(hr)) {
        dprintf("Wheel: SetCooperativeLevel failed: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInputDevice8_SetDataFormat(idz_di_dev, &c_dfDIJoystick);

    if (FAILED(hr)) {
        dprintf("Wheel: SetDataFormat failed: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInputDevice8_Acquire(idz_di_dev);

    if (FAILED(hr)) {
        dprintf("Wheel: Acquire failed: %08x\n", (int) hr);

        return hr;
    }

    dprintf("Wheel: DirectInput initialized\n");
    idz_di_try_fx();
    *backend = &idz_di_backend;

    return S_OK;
}

static BOOL idz_di_enum_callback(const DIDEVICEINSTANCEW *dev, void *ctx)
{
    IDirectInput8_CreateDevice(
            idz_di_api,
            &dev->guidInstance,
            &idz_di_dev,
            NULL);

    return DIENUM_STOP;
}

static void idz_di_try_fx(void)
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

    DWORD axis;
    LONG direction;
    DIEFFECT fx;
    DICONSTANTFORCE cf;
    HRESULT hr;

    dprintf("Wheel: Attempting to start force feedback (may take a sec)\n");

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
            idz_di_dev,
            &GUID_ConstantForce,
            &fx,
            &idz_di_fx,
            NULL);

    if (FAILED(hr)) {
        dprintf("Wheel: DirectInput force feedback unavailable: %08x\n",
                (int) hr);

        return;
    }

    hr = IDirectInputEffect_Start(idz_di_fx, INFINITE, 0);

    if (FAILED(hr)) {
        dprintf("Wheel: DirectInput force feedback start failed: %08x\n",
                (int) hr);

        return;
    }

    dprintf("Wheel: Force feedback initialized and set to zero\n");
}

static HRESULT idz_di_poll(DIJOYSTATE *out)
{
    HRESULT hr;
    MSG msg;

    memset(out, 0, sizeof(*out));

    if (idz_di_dev == NULL) {
        return E_UNEXPECTED;
    }

    /* Pump our dummy window's message queue just in case DirectInput or an
       IHV DirectInput driver somehow relies on it */

    while (PeekMessageW(&msg, idz_di_wnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hr = IDirectInputDevice8_GetDeviceState(idz_di_dev, sizeof(*out), out);

    if (FAILED(hr)) {
        dprintf("Wheel: I/O error: %08x\n", (int) hr);
    }

    /* JVS lacks a protocol for reporting hardware errors from poll command
       responses, so this ends up returning zeroed input state instead. */

    return hr;
}

static void idz_di_jvs_read_buttons(uint8_t *gamebtn_out)
{
    uint8_t gamebtn;
    DIJOYSTATE state;
    HRESULT hr;

    assert(gamebtn_out != NULL);

    hr = idz_di_poll(&state);

    if (FAILED(hr)) {
        return;
    }

    gamebtn = idz_di_decode_pov(state.rgdwPOV[0]);

    if (state.rgbButtons[2]) {
        gamebtn |= IDZ_IO_GAMEBTN_START;
    }

    if (state.rgbButtons[9]) {
        gamebtn |= IDZ_IO_GAMEBTN_VIEW_CHANGE;
    }

    *gamebtn_out = gamebtn;
}

static uint8_t idz_di_decode_pov(DWORD pov)
{
    switch (pov) {
        case 0:     return IDZ_IO_GAMEBTN_UP;
        case 4500:  return IDZ_IO_GAMEBTN_UP | IDZ_IO_GAMEBTN_RIGHT;
        case 9000:  return IDZ_IO_GAMEBTN_RIGHT;
        case 13500: return IDZ_IO_GAMEBTN_RIGHT | IDZ_IO_GAMEBTN_DOWN;
        case 18000: return IDZ_IO_GAMEBTN_DOWN;
        case 22500: return IDZ_IO_GAMEBTN_DOWN | IDZ_IO_GAMEBTN_RIGHT;
        case 27000: return IDZ_IO_GAMEBTN_LEFT;
        case 31500: return IDZ_IO_GAMEBTN_LEFT | IDZ_IO_GAMEBTN_UP;
        default:    return 0;
    }
}

static void idz_di_jvs_read_shifter(uint8_t *gear)
{
    DIJOYSTATE state;
    HRESULT hr;

    assert(gear != NULL);

    hr = idz_di_poll(&state);

    if (FAILED(hr)) {
        return;
    }

    if (state.rgbButtons[2]) {
        idz_shifter_reset();
    }

    idz_shifter_update(state.rgbButtons[0], state.rgbButtons[1]);

    *gear = idz_shifter_current_gear();
}

static void idz_di_jvs_read_analogs(struct idz_io_analog_state *out)
{
    DIJOYSTATE state;
    HRESULT hr;

    assert(out != NULL);

    hr = idz_di_poll(&state);

    if (FAILED(hr)) {
        return;
    }

    out->wheel = state.lX - 32768;
    out->accel = 65535 - state.lRz;
    out->brake = 65535 - state.lY;
}
