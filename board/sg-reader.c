#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "board/sg-led.h"
#include "board/sg-nfc.h"
#include "board/sg-reader.h"

#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT sg_reader_handle_irp_locked(
        struct sg_reader *reader,
        struct irp *irp);

void sg_reader_init(
        struct sg_reader *reader,
        unsigned int port_no,
        const struct sg_nfc_ops *nfc_ops,
        const struct sg_led_ops *led_ops,
        void *ops_ctx)
{
    assert(reader != NULL);
    assert(nfc_ops != NULL);
    assert(led_ops != NULL);

    sg_nfc_init(&reader->nfc, 0x00, nfc_ops, ops_ctx);
    sg_led_init(&reader->led, 0x08, led_ops, ops_ctx);

    InitializeCriticalSection(&reader->lock);

    uart_init(&reader->uart, port_no);
    reader->uart.written.bytes = reader->written_bytes;
    reader->uart.written.nbytes = sizeof(reader->written_bytes);
    reader->uart.readable.bytes = reader->readable_bytes;
    reader->uart.readable.nbytes = sizeof(reader->readable_bytes);
}

HRESULT sg_reader_handle_irp(struct sg_reader *reader, struct irp *irp)
{
    HRESULT hr;

    assert(reader != NULL);
    assert(irp != NULL);

    if (!uart_match_irp(&reader->uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&reader->lock);
    hr = sg_reader_handle_irp_locked(reader, irp);
    LeaveCriticalSection(&reader->lock);

    return hr;
}

static HRESULT sg_reader_handle_irp_locked(
        struct sg_reader *reader,
        struct irp *irp)
{
    HRESULT hr;

#if 0
    if (irp->op == IRP_OP_WRITE) {
        dprintf("WRITE:\n");
        dump_const_iobuf(&irp->write);
    }
#endif

#if 0
    if (irp->op == IRP_OP_READ) {
        dprintf("READ:\n");
        dump_iobuf(&reader->uart.readable);
    }
#endif

    hr = uart_handle_irp(&reader->uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    sg_nfc_transact(
            &reader->nfc,
            &reader->uart.readable,
            reader->uart.written.bytes,
            reader->uart.written.pos);

    sg_led_transact(
            &reader->led,
            &reader->uart.readable,
            reader->uart.written.bytes,
            reader->uart.written.pos);

    reader->uart.written.pos = 0;

    return hr;
}
