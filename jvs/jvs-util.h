#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

typedef HRESULT (*jvs_dispatch_fn_t)(
        void *ctx,
        struct const_iobuf *req,
        struct iobuf *resp);

void jvs_crack_request(
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp,
        uint8_t jvs_addr,
        jvs_dispatch_fn_t dispatch_fn,
        void *dispatch_ctx);
