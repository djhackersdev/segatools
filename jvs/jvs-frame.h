#pragma once

#include <windows.h>

#include <stddef.h>

#include "hook/iobuf.h"

HRESULT jvs_frame_decode(
        struct iobuf *dest,
        const void *bytes,
        size_t nbytes);

HRESULT jvs_frame_encode(
        struct iobuf *dest,
        const void *bytes,
        size_t nbytes);
