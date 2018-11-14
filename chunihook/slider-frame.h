#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

enum {
    SLIDER_FRAME_SYNC = 0xFF,
};

struct slider_hdr {
    uint8_t sync;
    uint8_t cmd;
    uint8_t nbytes;
};

HRESULT slider_frame_decode(struct iobuf *dest, struct iobuf *src);

HRESULT slider_frame_encode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes);
