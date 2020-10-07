#pragma once

#include <windows.h>

#include <stdint.h>

enum {
    /* System buttons in button[0] */

    IO4_BUTTON_TEST     = 1 << 9,
    IO4_BUTTON_SERVICE  = 1 << 6,
};

struct io4_config {
    bool enable;
};

struct io4_state {
    uint16_t adcs[8];
    uint16_t spinners[4];
    uint16_t chutes[2];
    uint16_t buttons[2];
};

struct io4_ops {
    HRESULT (*poll)(void *ctx, struct io4_state *state);
};

HRESULT io4_hook_init(
        const struct io4_config *cfg,
        const struct io4_ops *ops,
        void *ctx);
