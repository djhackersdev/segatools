#include <windows.h>
#include <xinput.h>

#include <stdbool.h>
#include <stdint.h>

#include "idzio/idzio.h"

static bool idz_io_coin;
static uint16_t idz_io_coins;
static bool idz_io_shifting;
static uint8_t idz_io_gear;

HRESULT idz_io_init(void)
{
    return S_OK;
}

void idz_io_jvs_read_buttons(uint8_t *opbtn_out, uint8_t *gamebtn_out)
{
    uint8_t opbtn;
    uint8_t gamebtn;
    XINPUT_STATE xi;
    WORD xb;

    opbtn = 0;
    gamebtn = 0;

    /* Update test/service buttons */

    if (GetAsyncKeyState('1')) {
        opbtn |= IDZ_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState('2')) {
        opbtn |= IDZ_IO_OPBTN_SERVICE;
    }

    /* Update gameplay buttons */

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

    *opbtn_out = opbtn;
    *gamebtn_out = gamebtn;
}

void idz_io_jvs_read_shifter(uint8_t *gear)
{
    bool shift_inc;
    bool shift_dec;
    XINPUT_STATE xi;
    WORD xb;

    memset(&xi, 0, sizeof(xi));
    XInputGetState(0, &xi);
    xb = xi.Gamepad.wButtons;

    if (xb & XINPUT_GAMEPAD_START) {
        idz_io_gear = 0; /* Reset to Neutral when start is pressed */
    }

    shift_inc = xb & (XINPUT_GAMEPAD_X | XINPUT_GAMEPAD_RIGHT_SHOULDER);
    shift_dec = xb & (XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_LEFT_SHOULDER);

    if (!idz_io_shifting) {
        if (shift_inc && idz_io_gear < 6) {
            idz_io_gear++;
        }

        if (shift_dec && idz_io_gear > 0) {
            idz_io_gear--;
        }
    }

    idz_io_shifting = shift_inc || shift_dec;
    *gear = idz_io_gear;
}

void idz_io_jvs_read_analogs(struct idz_io_analog_state *out)
{
    XINPUT_STATE xi;
    int left;
    int right;

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

void idz_io_jvs_read_coin_counter(uint16_t *out)
{
    if (GetAsyncKeyState('3')) {
        if (!idz_io_coin) {
            idz_io_coin = true;
            idz_io_coins++;
        }
    } else {
        idz_io_coin = false;
    }

    *out = idz_io_coins;
}
