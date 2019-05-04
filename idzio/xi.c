#include <windows.h>
#include <xinput.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "idzio/backend.h"
#include "idzio/idzio.h"
#include "idzio/xi.h"

#include "util/dprintf.h"

static void idz_xi_jvs_read_buttons(uint8_t *gamebtn_out);
static void idz_xi_jvs_read_shifter(uint8_t *gear);
static void idz_xi_jvs_read_analogs(struct idz_io_analog_state *out);

static const struct idz_io_backend idz_xi_backend = {
    .jvs_read_buttons   = idz_xi_jvs_read_buttons,
    .jvs_read_shifter   = idz_xi_jvs_read_shifter,
    .jvs_read_analogs   = idz_xi_jvs_read_analogs,
};

static bool idz_xi_shifting;
static uint8_t idz_xi_gear;

HRESULT idz_xi_init(const struct idz_io_backend **backend)
{
    assert(backend != NULL);

    dprintf("IDZ XI: Using XInput controller\n");
    *backend = &idz_xi_backend;

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

    if (xb & XINPUT_GAMEPAD_START) {
        gamebtn |= IDZ_IO_GAMEBTN_START;
    }

    if (xb & XINPUT_GAMEPAD_BACK) {
        gamebtn |= IDZ_IO_GAMEBTN_VIEW_CHANGE;
    }

    *gamebtn_out = gamebtn;
}

static void idz_xi_jvs_read_shifter(uint8_t *gear)
{
    bool shift_inc;
    bool shift_dec;
    XINPUT_STATE xi;
    WORD xb;

    assert(gear != NULL);

    memset(&xi, 0, sizeof(xi));
    XInputGetState(0, &xi);
    xb = xi.Gamepad.wButtons;

    if (xb & XINPUT_GAMEPAD_START) {
        idz_xi_gear = 0; /* Reset to Neutral when start is pressed */
    }

    shift_inc = xb & (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_RIGHT_SHOULDER);
    shift_dec = xb & (XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_LEFT_SHOULDER);

    if (!idz_xi_shifting) {
        if (shift_inc && idz_xi_gear < 6) {
            idz_xi_gear++;
        }

        if (shift_dec && idz_xi_gear > 0) {
            idz_xi_gear--;
        }
    }

    idz_xi_shifting = shift_inc || shift_dec;
    *gear = idz_xi_gear;
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

    out->wheel = (left + right) / 2;
    out->accel = xi.Gamepad.bRightTrigger << 8;
    out->brake = xi.Gamepad.bLeftTrigger << 8;
}
