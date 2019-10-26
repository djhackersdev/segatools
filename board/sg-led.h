#pragma once

#include <windows.h>

#include <stdint.h>

#include "hook/iobuf.h"

struct sg_led_ops {
    HRESULT (*reset)(void *ctx);
    void (*set_color)(void *ctx, uint8_t r, uint8_t g, uint8_t b);
};

struct sg_led {
    const struct sg_led_ops *ops;
    void *ops_ctx;
    uint8_t addr;
};

void sg_led_init(
        struct sg_led *led,
        uint8_t addr,
        const struct sg_led_ops *ops,
        void *ctx);

void sg_led_transact(
        struct sg_led *led,
        struct iobuf *res_frame,
        const void *req_bytes,
        size_t req_nbytes);
