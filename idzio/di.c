#include <windows.h>
#include <dinput.h>

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#include "idzio/backend.h"
#include "idzio/config.h"
#include "idzio/di.h"
#include "idzio/di-dev.h"
#include "idzio/idzio.h"
#include "idzio/shifter.h"
#include "idzio/wnd.h"

#include "util/dprintf.h"
#include "util/str.h"

struct idz_di_axis {
    wchar_t name[4];
    size_t off;
};

static HRESULT idz_di_config_apply(const struct idz_di_config *cfg);
static const struct idz_di_axis *idz_di_get_axis(const wchar_t *name);
static BOOL CALLBACK idz_di_enum_callback(
        const DIDEVICEINSTANCEW *dev,
        void *ctx);
static BOOL CALLBACK idz_di_enum_callback_shifter(
        const DIDEVICEINSTANCEW *dev,
        void *ctx);
static void idz_di_jvs_read_buttons(uint8_t *gamebtn_out);
static uint8_t idz_di_decode_pov(DWORD pov);
static void idz_di_jvs_read_shifter(uint8_t *gear);
static void idz_di_jvs_read_shifter_pos(uint8_t *gear);
static void idz_di_jvs_read_shifter_virt(uint8_t *gear);
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
static IDirectInputDevice8W *idz_di_shifter;
static IDirectInputEffect *idz_di_fx;
static size_t idz_di_off_brake;
static size_t idz_di_off_accel;
static uint8_t idz_di_shift_dn;
static uint8_t idz_di_shift_up;
static uint8_t idz_di_view_chg;
static uint8_t idz_di_start;
static uint8_t idz_di_gear[6];
static bool idz_di_reverse_brake_axis;
static bool idz_di_reverse_accel_axis;

HRESULT idz_di_init(
        const struct idz_di_config *cfg,
        HINSTANCE inst,
        const struct idz_io_backend **backend)
{
    HRESULT hr;
    HMODULE dinput8;
    HRESULT (WINAPI *api_entry)(HINSTANCE,DWORD,REFIID,LPVOID *,LPUNKNOWN);
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
       particularly useful. idzhook shorts this out by redirecting dinput8.dll
       to a no-op implementation of DirectInput. However, idzio does need to
       talk to the real operating system implementation of DirectInput without
       the stub DLL interfering, so build a path to
       C:\Windows\System32\dinput.dll here. */

    dll_path[0] = L'\0';
    path_pos = GetSystemDirectoryW(dll_path, _countof(dll_path));
    wcscat_s(
            dll_path + path_pos,
            _countof(dll_path) - path_pos,
            L"\\dinput8.dll");

    dinput8 = LoadLibraryW(dll_path);

    if (dinput8 == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("DirectInput: LoadLibrary failed: %08x\n", (int) hr);

        return hr;
    }

    api_entry = (void *) GetProcAddress(dinput8, "DirectInput8Create");

    if (api_entry == NULL) {
        dprintf("DirectInput: GetProcAddress failed\n");

        return E_FAIL;
    }

    hr = api_entry(
            inst,
            DIRECTINPUT_VERSION,
            &IID_IDirectInput8W,
            (void **) &idz_di_api,
            NULL);

    if (FAILED(hr)) {
        dprintf("DirectInput: API create failed: %08x\n", (int) hr);

        return hr;
    }

    hr = IDirectInput8_EnumDevices(
            idz_di_api,
            DI8DEVCLASS_GAMECTRL,
            idz_di_enum_callback,
            (void *) cfg,
            DIEDFL_ATTACHEDONLY);

    if (FAILED(hr)) {
        dprintf("DirectInput: EnumDevices failed: %08x\n", (int) hr);

        return hr;
    }

    if (idz_di_dev == NULL) {
        dprintf("Wheel: Controller not found\n");

        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    hr = idz_di_dev_start(idz_di_dev, idz_di_wnd);

    if (FAILED(hr)) {
        return hr;
    }

    idz_di_dev_start_fx(idz_di_dev, &idz_di_fx);

    if (cfg->shifter_name[0] != L'\0') {
        hr = IDirectInput8_EnumDevices(
                idz_di_api,
                DI8DEVCLASS_GAMECTRL,
                idz_di_enum_callback_shifter,
                (void *) cfg,
                DIEDFL_ATTACHEDONLY);

        if (FAILED(hr)) {
            dprintf("DirectInput: EnumDevices failed: %08x\n", (int) hr);

            return hr;
        }

        if (idz_di_dev == NULL) {
            dprintf("Shifter: Controller not found\n");

            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        hr = idz_di_dev_start(idz_di_shifter, idz_di_wnd);

        if (FAILED(hr)) {
            return hr;
        }
    }

    dprintf("DirectInput: Controller initialized\n");

    *backend = &idz_di_backend;

    return S_OK;
}

static HRESULT idz_di_config_apply(const struct idz_di_config *cfg)
{
    const struct idz_di_axis *brake_axis;
    const struct idz_di_axis *accel_axis;
    int i;

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

    for (i = 0 ; i < 6 ; i++) {
        if (cfg->gear[i] > 32) {
            dprintf("Shifter: Invalid gear %i button: %i\n",
                    i + 1,
                    cfg->gear[i]);

            return E_INVALIDARG;
        }
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
    dprintf("Wheel: Reverse Brake Axis  : %i\n", cfg->reverse_brake_axis);
    dprintf("Wheel: Reverse Accel Axis  : %i\n", cfg->reverse_accel_axis);
    dprintf("Wheel: ---  End  configuration ---\n");

    if (cfg->shifter_name[0] != L'\0') {
        dprintf("Shifter: --- Begin configuration ---\n");
        dprintf("Shifter: Device name . . . : Contains \"%S\"\n",
                cfg->shifter_name);
        dprintf("Shifter: Gear buttons  . . : %i %i %i %i %i %i\n",
                cfg->gear[0],
                cfg->gear[1],
                cfg->gear[2],
                cfg->gear[3],
                cfg->gear[4],
                cfg->gear[5]);
        dprintf("Shifter: ---  End  configuration ---\n");
    }

    idz_di_off_brake = accel_axis->off;
    idz_di_off_accel = brake_axis->off;
    idz_di_start = cfg->start;
    idz_di_view_chg = cfg->view_chg;
    idz_di_shift_dn = cfg->shift_dn;
    idz_di_shift_up = cfg->shift_up;
    idz_di_reverse_brake_axis = cfg->reverse_brake_axis;
    idz_di_reverse_accel_axis = cfg->reverse_accel_axis;

    for (i = 0 ; i < 6 ; i++) {
        idz_di_gear[i] = cfg->gear[i];
    }

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

static BOOL CALLBACK idz_di_enum_callback(
        const DIDEVICEINSTANCEW *dev,
        void *ctx)
{
    const struct idz_di_config *cfg;
    HRESULT hr;

    cfg = ctx;

    if (wcsstr(dev->tszProductName, cfg->device_name) == NULL) {
        return DIENUM_CONTINUE;
    }

    dprintf("Wheel: Using DirectInput device \"%S\"\n", dev->tszProductName);

    hr = IDirectInput8_CreateDevice(
            idz_di_api,
            &dev->guidInstance,
            &idz_di_dev,
            NULL);

    if (FAILED(hr)) {
        dprintf("Wheel: CreateDevice failed: %08x\n", (int) hr);
    }

    return DIENUM_STOP;
}

static BOOL CALLBACK idz_di_enum_callback_shifter(
        const DIDEVICEINSTANCEW *dev,
        void *ctx)
{
    const struct idz_di_config *cfg;
    HRESULT hr;

    cfg = ctx;

    if (wcsstr(dev->tszProductName, cfg->shifter_name) == NULL) {
        return DIENUM_CONTINUE;
    }

    dprintf("Shifter: Using DirectInput device \"%S\"\n", dev->tszProductName);

    hr = IDirectInput8_CreateDevice(
            idz_di_api,
            &dev->guidInstance,
            &idz_di_shifter,
            NULL);

    if (FAILED(hr)) {
        dprintf("Shifter: CreateDevice failed: %08x\n", (int) hr);
    }

    return DIENUM_STOP;
}

static void idz_di_jvs_read_buttons(uint8_t *gamebtn_out)
{
    union idz_di_state state;
    uint8_t gamebtn;
    HRESULT hr;

    assert(gamebtn_out != NULL);

    hr = idz_di_dev_poll(idz_di_dev, idz_di_wnd, &state);

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
    assert(gear != NULL);

    if (idz_di_shifter != NULL) {
        idz_di_jvs_read_shifter_pos(gear);
    } else {
        idz_di_jvs_read_shifter_virt(gear);
    }
}

static void idz_di_jvs_read_shifter_pos(uint8_t *out)
{
    union idz_di_state state;
    uint8_t btn_no;
    uint8_t gear;
    uint8_t i;
    HRESULT hr;

    assert(out != NULL);
    assert(idz_di_shifter != NULL);

    hr = idz_di_dev_poll(idz_di_shifter, idz_di_wnd, &state);

    if (FAILED(hr)) {
        return;
    }

    gear = 0;

    for (i = 0 ; i < 6 ; i++) {
        btn_no = idz_di_gear[i];

        if (btn_no && state.st.rgbButtons[btn_no - 1]) {
            gear = i + 1;
        }
    }

    *out = gear;
}

static void idz_di_jvs_read_shifter_virt(uint8_t *gear)
{
    union idz_di_state state;
    bool shift_dn;
    bool shift_up;
    HRESULT hr;

    assert(gear != NULL);

    hr = idz_di_dev_poll(idz_di_dev, idz_di_wnd, &state);

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

    hr = idz_di_dev_poll(idz_di_dev, idz_di_wnd, &state);

    if (FAILED(hr)) {
        return;
    }

    brake = (LONG *) &state.bytes[idz_di_off_brake];
    accel = (LONG *) &state.bytes[idz_di_off_accel];

    out->wheel = state.st.lX - 32768;

    if (idz_di_reverse_brake_axis) {
        out->brake = *brake;
    } else {
        out->brake = 65535 - *brake;
    }

    if (idz_di_reverse_accel_axis) {
        out->accel = *accel;
    } else {
        out->accel = 65535 - *accel;
    }
}
