#include <windows.h>
#include <dinput.h>

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "idzio/backend.h"
#include "idzio/config.h"
#include "idzio/di.h"
#include "idzio/idzio.h"
#include "idzio/shifter.h"
#include "idzio/wnd.h"

#include "util/dprintf.h"
#include "util/str.h"

struct idz_di_axis {
    wchar_t name[4];
    size_t off;
};

union idz_di_state {
    DIJOYSTATE st;
    uint8_t bytes[sizeof(DIJOYSTATE)];
};

static HRESULT idz_di_config_apply(const struct idz_di_config *cfg);
static const struct idz_di_axis *idz_di_get_axis(const wchar_t *name);
static BOOL idz_di_enum_callback(const DIDEVICEINSTANCEW *dev, void *ctx);
static void idz_di_try_fx(void);
static HRESULT idz_di_poll(union idz_di_state *state);
static void idz_di_jvs_read_buttons(uint8_t *gamebtn_out);
static uint8_t idz_di_decode_pov(DWORD pov);
static void idz_di_jvs_read_shifter(uint8_t *gear);
static void idz_di_jvs_read_analogs(struct idz_io_analog_state *out);

static const struct idz_di_axis idz_di_axes[] = {
    /* Just map DIJOYSTATE for now, we can map DIJOYSTATE2 later if needed */
    { .name = L"X",     .off = DIJOFS_X },
    { .name = L"Y",     .off = DIJOFS_Y },
    { .name = L"Z",     .off = DIJOFS_Z },
    { .name = L"RX",    .off = DIJOFS_RX },
    { .name = L"RY",    .off = DIJOFS_RY },
    { .name = L"RZ",    .off = DIJOFS_RZ },
    { .name = L"U",     .off = DIJOFS_SLIDER(0) },
    { .name = L"V",     .off = DIJOFS_SLIDER(1) },
};

static const struct idz_io_backend idz_di_backend = {
    .jvs_read_buttons   = idz_di_jvs_read_buttons,
    .jvs_read_shifter   = idz_di_jvs_read_shifter,
    .jvs_read_analogs   = idz_di_jvs_read_analogs,
};

static HWND idz_di_wnd;
static IDirectInput8W *idz_di_api;
static IDirectInputDevice8W *idz_di_dev;
static IDirectInputEffect *idz_di_fx;
static size_t idz_di_off_brake;
static size_t idz_di_off_accel;
static uint8_t idz_di_shift_dn;
static uint8_t idz_di_shift_up;
static uint8_t idz_di_view_chg;
static uint8_t idz_di_start;

HRESULT idz_di_init(
        const struct idz_di_config *cfg,
        HINSTANCE inst,
        const struct idz_io_backend **backend)
{
    HRESULT hr;
    HMODULE dinput8;
    HRESULT WINAPI (*api_entry)(HINSTANCE,DWORD,REFIID,LPVOID *,LPUNKNOWN);
    wchar_t dll_path[MAX_PATH];
    UINT path_pos;

    assert(cfg != NULL);
    assert(backend != NULL);

    *backend = NULL;

    hr = idz_di_config_apply(cfg);

    if (FAILED(hr)) {
        return hr;
    }

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
            (void *) cfg,
            DIEDFL_ATTACHEDONLY);

    if (FAILED(hr)) {
        dprintf("Wheel: EnumDevices failed: %08x\n", (int) hr);

        return hr;
    }

    if (idz_di_dev == NULL) {
        dprintf("Wheel: Controller not found\n");

        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
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

static HRESULT idz_di_config_apply(const struct idz_di_config *cfg)
{
    const struct idz_di_axis *brake_axis;
    const struct idz_di_axis *accel_axis;

    brake_axis = idz_di_get_axis(cfg->brake_axis);
    accel_axis = idz_di_get_axis(cfg->accel_axis);

    if (brake_axis == NULL) {
        dprintf("Wheel: Invalid brake axis: %S\n", cfg->brake_axis);

        return E_INVALIDARG;
    }

    if (accel_axis == NULL) {
        dprintf("Wheel: Invalid accel axis: %S\n", cfg->accel_axis);

        return E_INVALIDARG;
    }

    if (cfg->start > 32) {
        dprintf("Wheel: Invalid start button: %i\n", cfg->start);

        return E_INVALIDARG;
    }

    if (cfg->view_chg > 32) {
        dprintf("Wheel: Invalid view change button: %i\n", cfg->view_chg);

        return E_INVALIDARG;
    }

    if (cfg->shift_dn > 32) {
        dprintf("Wheel: Invalid shift down button: %i\n", cfg->shift_dn);

        return E_INVALIDARG;
    }

    if (cfg->shift_up > 32) {
        dprintf("Wheel: Invalid shift up button: %i\n", cfg->shift_up);

        return E_INVALIDARG;
    }

    /* Print some debug output to make sure config works... */

    dprintf("Wheel: --- Begin configuration ---\n");
    dprintf("Wheel: Device name . . . . : Contains \"%S\"\n",
            cfg->device_name);
    dprintf("Wheel: Brake axis  . . . . : %S\n", accel_axis->name);
    dprintf("Wheel: Accelerator axis  . : %S\n", brake_axis->name);
    dprintf("Wheel: Start button  . . . : %i\n", cfg->start);
    dprintf("Wheel: View Change button  : %i\n", cfg->view_chg);
    dprintf("Wheel: Shift Down button . : %i\n", cfg->shift_dn);
    dprintf("Wheel: Shift Up button . . : %i\n", cfg->shift_up);
    dprintf("Wheel: ---  End  configuration ---\n");

    idz_di_off_brake = accel_axis->off;
    idz_di_off_accel = brake_axis->off;
    idz_di_start = cfg->start;
    idz_di_view_chg = cfg->view_chg;
    idz_di_shift_dn = cfg->shift_dn;
    idz_di_shift_up = cfg->shift_up;

    return S_OK;
}

static const struct idz_di_axis *idz_di_get_axis(const wchar_t *name)
{
    const struct idz_di_axis *axis;
    size_t i;

    for (i = 0 ; i < _countof(idz_di_axes) ; i++) {
        axis = &idz_di_axes[i];

        if (wstr_ieq(name, axis->name)) {
            return axis;
        }
    }

    return NULL;
}

static BOOL idz_di_enum_callback(const DIDEVICEINSTANCEW *dev, void *ctx)
{
    const struct idz_di_config *cfg;

    cfg = ctx;

    if (wcsstr(dev->tszProductName, cfg->device_name) == NULL) {
        return DIENUM_CONTINUE;
    }

    dprintf("Wheel: Using DirectInput device \"%S\"\n", dev->tszProductName);

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

static HRESULT idz_di_poll(union idz_di_state *out)
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

    hr = IDirectInputDevice8_GetDeviceState(
            idz_di_dev,
            sizeof(out->st),
            &out->st);

    if (FAILED(hr)) {
        dprintf("Wheel: I/O error: %08x\n", (int) hr);
    }

    /* JVS lacks a protocol for reporting hardware errors from poll command
       responses, so this ends up returning zeroed input state instead. */

    return hr;
}

static void idz_di_jvs_read_buttons(uint8_t *gamebtn_out)
{
    union idz_di_state state;
    uint8_t gamebtn;
    HRESULT hr;

    assert(gamebtn_out != NULL);

    hr = idz_di_poll(&state);

    if (FAILED(hr)) {
        return;
    }

    gamebtn = idz_di_decode_pov(state.st.rgdwPOV[0]);

    if (idz_di_start && state.st.rgbButtons[idz_di_start - 1]) {
        gamebtn |= IDZ_IO_GAMEBTN_START;
    }

    if (idz_di_view_chg && state.st.rgbButtons[idz_di_view_chg - 1]) {
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
    union idz_di_state state;
    bool shift_dn;
    bool shift_up;
    HRESULT hr;

    assert(gear != NULL);

    hr = idz_di_poll(&state);

    if (FAILED(hr)) {
        return;
    }

    if (idz_di_shift_dn) {
        shift_dn = state.st.rgbButtons[idz_di_shift_dn - 1];
    } else {
        shift_dn = false;
    }

    if (idz_di_shift_up) {
        shift_up = state.st.rgbButtons[idz_di_shift_up - 1];
    } else {
        shift_up = false;
    }

    idz_shifter_update(shift_dn, shift_up);

    *gear = idz_shifter_current_gear();
}

static void idz_di_jvs_read_analogs(struct idz_io_analog_state *out)
{
    union idz_di_state state;
    const LONG *brake;
    const LONG *accel;
    HRESULT hr;

    assert(out != NULL);

    hr = idz_di_poll(&state);

    if (FAILED(hr)) {
        return;
    }

    brake = (LONG *) &state.bytes[idz_di_off_brake];
    accel = (LONG *) &state.bytes[idz_di_off_accel];

    out->wheel = state.st.lX - 32768;
    out->brake = 65535 - *brake;
    out->accel = 65535 - *accel;
}
