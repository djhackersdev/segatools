#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

HRESULT sg_frame_decode(
        struct iobuf *dest,
        const uint8_t *bytes,
        size_t nbytes);

HRESULT sg_frame_encode(struct iobuf *dest, const void *ptr, size_t nbytes);
