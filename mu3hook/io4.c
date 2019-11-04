#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "board/io4.h"

#include "mu3io/mu3io.h"

#include "util/dprintf.h"

static HRESULT mu3_io4_poll(void *ctx, struct io4_state *state);

static const struct io4_ops mu3_io4_ops = {
    .poll = mu3_io4_poll,
};

HRESULT mu3_io4_hook_init(void)
{
    HRESULT hr;

    hr = io4_hook_init(&mu3_io4_ops, NULL);

    if (FAILED(hr)) {
        return hr;
    }

    return mu3_io_init();
}

static HRESULT mu3_io4_poll(void *ctx, struct io4_state *state)
{
    uint8_t opbtn;
    uint8_t left;
    uint8_t right;
    int16_t lever;
    HRESULT hr;

    memset(state, 0, sizeof(*state));

    hr = mu3_io_poll();

    if (FAILED(hr)) {
        return hr;
    }

    opbtn = 0;
    left = 0;
    right = 0;
    lever = 0;

    mu3_io_get_opbtns(&opbtn);
    mu3_io_get_gamebtns(&left, &right);
    mu3_io_get_lever(&lever);

    if (opbtn & MU3_IO_OPBTN_TEST) {
        state->buttons[0] |= IO4_BUTTON_TEST;
    }

    if (opbtn & MU3_IO_OPBTN_SERVICE) {
        state->buttons[0] |= IO4_BUTTON_SERVICE;
    }

    if (left & MU3_IO_GAMEBTN_1) {
        state->buttons[0] |= 1 << 0;
    }

    if (left & MU3_IO_GAMEBTN_2) {
        state->buttons[0] |= 1 << 5;
    }

    if (left & MU3_IO_GAMEBTN_3) {
        state->buttons[0] |= 1 << 4;
    }

    if (right & MU3_IO_GAMEBTN_1) {
        state->buttons[0] |= 1 << 1;
    }

    if (right & MU3_IO_GAMEBTN_2) {
        state->buttons[1] |= 1 << 0;
    }

    if (right & MU3_IO_GAMEBTN_3) {
        state->buttons[0] |= 1 << 15;
    }

    if (left & MU3_IO_GAMEBTN_MENU) {
        state->buttons[1] |= 1 << 14;
    }

    if (right & MU3_IO_GAMEBTN_MENU) {
        state->buttons[0] |= 1 << 13;
    }

    if (!(left & MU3_IO_GAMEBTN_SIDE)) {
        state->buttons[1] |= 1 << 15;   /* L-Side, active-low */
    }

    if (!(right & MU3_IO_GAMEBTN_SIDE)) {
        state->buttons[0] |= 1 << 14;   /* R-Side, active-low */
    }

    /* Lever increases right-to-left, not left-to-right.

       Use 0x7FFF as the center point instead of 0x8000; the latter would
       overflow when the lever pos is INT16_MIN. */

    state->adcs[0] = 0x7FFF - lever;

    return S_OK;
}
