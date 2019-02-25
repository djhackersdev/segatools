#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

struct sg_nfc_ops {
    HRESULT (*mifare_poll)(void *ctx, uint32_t *uid);
    HRESULT (*mifare_read_luid)(
            void *ctx,
            uint32_t uid,
            uint8_t *luid,
            size_t nbytes);
};

struct sg_nfc {
    const struct sg_nfc_ops *ops;
    void *ops_ctx;
    uint8_t addr;
};

void sg_nfc_init(
        struct sg_nfc *nfc,
        uint8_t addr,
        const struct sg_nfc_ops *ops,
        void *ops_ctx);

void sg_nfc_transact(
        struct sg_nfc *nfc,
        struct iobuf *resp_frame,
        const void *req_bytes,
        size_t req_nbytes);
