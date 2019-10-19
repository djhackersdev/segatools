#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

#include "iccard/felica.h"
#include "iccard/mifare.h"

struct sg_nfc_ops {
    HRESULT (*poll)(void *ctx);
    HRESULT (*get_aime_id)(void *ctx, uint8_t *luid, size_t nbytes);
    HRESULT (*get_felica_id)(void *ctx, uint64_t *IDm);

    // TODO Banapass, AmuseIC
};

struct sg_nfc {
    const struct sg_nfc_ops *ops;
    void *ops_ctx;
    uint8_t addr;
    struct felica felica;
    struct mifare mifare;
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
