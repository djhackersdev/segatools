#pragma once

#include <windows.h>

#include <stdbool.h>
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

HRESULT sg_reader_init(
        struct sg_reader *reader,
        unsigned int port_no);

bool sg_reader_match_irp(const struct sg_reader *reader, const struct irp *irp);

HRESULT sg_reader_handle_irp(struct sg_reader *reader, struct irp *irp);
