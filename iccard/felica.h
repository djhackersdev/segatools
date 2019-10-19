#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

enum {
    FELICA_CMD_POLL             = 0x00,
    FELICA_CMD_GET_SYSTEM_CODE  = 0x0c,
    FELICA_CMD_NDA_A4           = 0xa4,
};

struct felica {
    uint64_t IDm;
    uint64_t PMm;
    uint16_t system_code;
};

HRESULT felica_transact(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res);

uint64_t felica_get_generic_PMm(void);
