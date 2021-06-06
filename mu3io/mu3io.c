#include <windows.h>
#include <xinput.h>

#include <limits.h>
#include <stdint.h>

#include "mu3io/mu3io.h"

static uint8_t mu3_opbtn;
static uint8_t mu3_left_btn;
static uint8_t mu3_right_btn;
static int16_t mu3_lever_pos;
static int16_t mu3_lever_xpos;

uint16_t mu3_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT mu3_io_init(void)
{
    return S_OK;
}

HRESULT mu3_io_poll(void)
{
    int lever;
    int xlever;
    XINPUT_STATE xi;
    WORD xb;

    mu3_opbtn = 0;
    mu3_left_btn = 0;
    mu3_right_btn = 0;

    if (GetAsyncKeyState('1') & 0x8000) {
        mu3_opbtn |= MU3_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState('2') & 0x8000) {
        mu3_opbtn |= MU3_IO_OPBTN_SERVICE;
    }

    memset(&xi, 0, sizeof(xi));
    XInputGetState(0, &xi);
    xb = xi.Gamepad.wButtons;

    if (xb & XINPUT_GAMEPAD_DPAD_LEFT) {
        mu3_left_btn |= MU3_IO_GAMEBTN_1;
    }

    if (xb & XINPUT_GAMEPAD_DPAD_UP) {
        mu3_left_btn |= MU3_IO_GAMEBTN_2;
    }

    if (xb & XINPUT_GAMEPAD_DPAD_RIGHT) {
        mu3_left_btn |= MU3_IO_GAMEBTN_3;
    }

    if (xb & XINPUT_GAMEPAD_X) {
        mu3_right_btn |= MU3_IO_GAMEBTN_1;
    }

    if (xb & XINPUT_GAMEPAD_Y) {
        mu3_right_btn |= MU3_IO_GAMEBTN_2;
    }

    if (xb & XINPUT_GAMEPAD_B) {
        mu3_right_btn |= MU3_IO_GAMEBTN_3;
    }

    if (xb & XINPUT_GAMEPAD_BACK) {
        mu3_left_btn |= MU3_IO_GAMEBTN_MENU;
    }

    if (xb & XINPUT_GAMEPAD_START) {
        mu3_right_btn |= MU3_IO_GAMEBTN_MENU;
    }

    if (xb & XINPUT_GAMEPAD_LEFT_SHOULDER) {
        mu3_left_btn |= MU3_IO_GAMEBTN_SIDE;
    }

    if (xb & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
        mu3_right_btn |= MU3_IO_GAMEBTN_SIDE;
    }

    lever = mu3_lever_pos;

    if (abs(xi.Gamepad.sThumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        lever += xi.Gamepad.sThumbLX / 24;
    }

    if (abs(xi.Gamepad.sThumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        lever += xi.Gamepad.sThumbRX / 24;
    }

    if (lever < INT16_MIN) {
        lever = INT16_MIN;
    }

    if (lever > INT16_MAX) {
        lever = INT16_MAX;
    }

    mu3_lever_pos = lever;

    xlever = mu3_lever_pos
                    - xi.Gamepad.bLeftTrigger * 64
                    + xi.Gamepad.bRightTrigger * 64;

    if (xlever < INT16_MIN) {
        xlever = INT16_MIN;
    }

    if (xlever > INT16_MAX) {
        xlever = INT16_MAX;
    }

    mu3_lever_xpos = xlever;

    return S_OK;
}

void mu3_io_get_opbtns(uint8_t *opbtn)
{
    if (opbtn != NULL) {
        *opbtn = mu3_opbtn;
    }
}

void mu3_io_get_gamebtns(uint8_t *left, uint8_t *right)
{
    if (left != NULL) {
        *left = mu3_left_btn;
    }

    if (right != NULL ){
        *right = mu3_right_btn;
    }
}

void mu3_io_get_lever(int16_t *pos)
{
    if (pos != NULL) {
        *pos = mu3_lever_xpos;
    }
}
