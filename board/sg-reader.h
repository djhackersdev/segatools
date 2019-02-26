#pragma once

#include <windows.h>

#include <stdint.h>

#include "board/sg-led.h"
#include "board/sg-nfc.h"

#include "hooklib/uart.h"

struct sg_reader {
    CRITICAL_SECTION lock;
    struct uart uart;
    uint8_t written_bytes[520];
    uint8_t readable_bytes[520];
    struct sg_nfc nfc;
    struct sg_led led;
};

void sg_reader_init(
        struct sg_reader *reader,
        unsigned int port_no,
        const struct sg_nfc_ops *nfc_ops,
        const struct sg_led_ops *led_ops,
        void *ops_ctx);

HRESULT sg_reader_handle_irp(struct sg_reader *reader, struct irp *irp);
