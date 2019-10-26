#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

struct sg_header {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
};

struct sg_req_header {
    struct sg_header hdr;
    uint8_t payload_len;
};

struct sg_res_header {
    struct sg_header hdr;
    uint8_t status;
    uint8_t payload_len;
};

typedef HRESULT (*sg_dispatch_fn_t)(
        void *ctx,
        const void *req,
        void *res);

void sg_req_transact(
        struct iobuf *res_frame,
        const uint8_t *req_bytes,
        size_t req_nbytes,
        sg_dispatch_fn_t dispatch,
        void *ctx);

void sg_res_init(
        struct sg_res_header *res,
        const struct sg_req_header *req,
        size_t payload_len);
