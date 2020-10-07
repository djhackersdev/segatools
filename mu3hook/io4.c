#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "board/io4.h"

#include "mu3hook/mu3-dll.h"

#include "util/dprintf.h"

static HRESULT mu3_io4_poll(void *ctx, struct io4_state *state);

static const struct io4_ops mu3_io4_ops = {
    .poll = mu3_io4_poll,
};

HRESULT mu3_io4_hook_init(const struct io4_config *cfg)
{
    HRESULT hr;

    assert(mu3_dll.init != NULL);

    hr = io4_hook_init(cfg, &mu3_io4_ops, NULL);

    if (FAILED(hr)) {
        return hr;
    }

    return mu3_dll.init();
}

static HRESULT mu3_io4_poll(void *ctx, struct io4_state *state)
{
    uint8_t opbtn;
    uint8_t left;
    uint8_t right;
    int16_t lever;
    HRESULT hr;

    assert(mu3_dll.poll != NULL);
    assert(mu3_dll.get_opbtns != NULL);
    assert(mu3_dll.get_gamebtns != NULL);
    assert(mu3_dll.get_lever != NULL);

    memset(state, 0, sizeof(*state));

    hr = mu3_dll.poll();

    if (FAILED(hr)) {
        return hr;
    }

    opbtn = 0;
    left = 0;
    right = 0;
    lever = 0;

    mu3_dll.get_opbtns(&opbtn);
    mu3_dll.get_gamebtns(&left, &right);
    mu3_dll.get_lever(&lever);

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
