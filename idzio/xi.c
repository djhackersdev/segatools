#include <windows.h>
#include <xinput.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "idzio/backend.h"
#include "idzio/config.h"
#include "idzio/idzio.h"
#include "idzio/shifter.h"
#include "idzio/xi.h"

#include "util/dprintf.h"

static void idz_xi_jvs_read_buttons(uint8_t *gamebtn_out);
static void idz_xi_jvs_read_shifter(uint8_t *gear);
static void idz_xi_jvs_read_analogs(struct idz_io_analog_state *out);

static HRESULT idz_xi_config_apply(const struct idz_xi_config *cfg);

static const struct idz_io_backend idz_xi_backend = {
    .jvs_read_buttons   = idz_xi_jvs_read_buttons,
    .jvs_read_shifter   = idz_xi_jvs_read_shifter,
    .jvs_read_analogs   = idz_xi_jvs_read_analogs,
};

static bool idz_xi_single_stick_steering;

HRESULT idz_xi_init(const struct idz_xi_config *cfg, const struct idz_io_backend **backend)
{
    HRESULT hr;
    assert(cfg != NULL);
    assert(backend != NULL);

    hr = idz_xi_config_apply(cfg);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("XInput: Using XInput controller\n");
    *backend = &idz_xi_backend;

    return S_OK;
}

static HRESULT idz_xi_config_apply(const struct idz_xi_config *cfg)
{
    dprintf("XInput: --- Begin configuration ---\n");
    dprintf("XInput: Single Stick Steering : %i\n", cfg->single_stick_steering);
    dprintf("XInput: ---  End  configuration ---\n");

    idz_xi_single_stick_steering = cfg->single_stick_steering;

    return S_OK;
}

static void idz_xi_jvs_read_buttons(uint8_t *gamebtn_out)
{
    uint8_t gamebtn;
    XINPUT_STATE xi;
    WORD xb;

    assert(gamebtn_out != NULL);

    gamebtn = 0;

    memset(&xi, 0, sizeof(xi));
    XInputGetState(0, &xi);
    xb = xi.Gamepad.wButtons;

    if (xb & XINPUT_GAMEPAD_DPAD_UP) {
        gamebtn |= IDZ_IO_GAMEBTN_UP;
    }

    if (xb & XINPUT_GAMEPAD_DPAD_DOWN) {
        gamebtn |= IDZ_IO_GAMEBTN_DOWN;
    }

    if (xb & XINPUT_GAMEPAD_DPAD_LEFT) {
        gamebtn |= IDZ_IO_GAMEBTN_LEFT;
    }

    if (xb & XINPUT_GAMEPAD_DPAD_RIGHT) {
        gamebtn |= IDZ_IO_GAMEBTN_RIGHT;
    }

    if (xb & (XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_A)) {
        gamebtn |= IDZ_IO_GAMEBTN_START;
    }

    if (xb & (XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_B)) {
        gamebtn |= IDZ_IO_GAMEBTN_VIEW_CHANGE;
    }

    *gamebtn_out = gamebtn;
}

static void idz_xi_jvs_read_shifter(uint8_t *gear)
{
    bool shift_dn;
    bool shift_up;
    XINPUT_STATE xi;
    WORD xb;

    assert(gear != NULL);

    memset(&xi, 0, sizeof(xi));
    XInputGetState(0, &xi);
    xb = xi.Gamepad.wButtons;

    if (xb & XINPUT_GAMEPAD_START) {
        /* Reset to Neutral when start is pressed */
        idz_shifter_reset();
    }

    shift_dn = xb & (XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_LEFT_SHOULDER);
    shift_up = xb & (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_RIGHT_SHOULDER);

    idz_shifter_update(shift_dn, shift_up);

    *gear = idz_shifter_current_gear();
}

static void idz_xi_jvs_read_analogs(struct idz_io_analog_state *out)
{
    XINPUT_STATE xi;
    int left;
    int right;

    assert(out != NULL);

    memset(&xi, 0, sizeof(xi));
    XInputGetState(0, &xi);

    left = xi.Gamepad.sThumbLX;

    if (left < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        left += XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    } else if (left > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        left -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    } else {
        left = 0;
    }

    right = xi.Gamepad.sThumbRX;

    if (right < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        right += XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    } else if (right > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        right -= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    } else {
        right = 0;
    }

    if(idz_xi_single_stick_steering) {
        out->wheel = left;
    } else {
        out->wheel = (left + right) / 2;
    }

    out->accel = xi.Gamepad.bRightTrigger << 8;
    out->brake = xi.Gamepad.bLeftTrigger << 8;
}
