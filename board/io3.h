#pragma once

#include <stdint.h>

#include "jvs/jvs-bus.h"

struct io3_switch_state {
    /* Note: this struct is host-endian. The IO3 emulator handles the conversion
       to protocol-endian. */

    uint8_t system;
    uint16_t p1;
    uint16_t p2;
};

struct io3_ops {
    void (*reset)(void *ctx);
    void (*write_gpio)(void *ctx, uint32_t state);
    void (*read_switches)(void *ctx, struct io3_switch_state *out);
    uint16_t (*read_analog)(void *ctx, uint8_t analog_no);
    uint16_t (*read_coin_counter)(void *ctx, uint8_t slot_no);
};

struct io3 {
    struct jvs_node jvs;
    uint8_t addr;
    const struct io3_ops *ops;
    void *ops_ctx;
};

void io3_init(
        struct io3 *io3,
        struct jvs_node *next,
        const struct io3_ops *ops,
        void *ops_ctx);
