#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "aimeio/aimeio.h"

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

static HRESULT sg_reader_mifare_poll(void *ctx, uint32_t *uid);

static HRESULT sg_reader_mifare_read_luid(
        void *ctx,
        uint32_t uid,
        uint8_t *luid,
        size_t luid_size);

static void sg_reader_led_set_color(void *ctx, uint8_t r, uint8_t g, uint8_t b);

static const struct sg_nfc_ops sg_reader_nfc_ops = {
    .mifare_poll        = sg_reader_mifare_poll,
    .mifare_read_luid   = sg_reader_mifare_read_luid,
};

static const struct sg_led_ops sg_reader_led_ops = {
    .set_color          = sg_reader_led_set_color,
};

HRESULT sg_reader_init(
        struct sg_reader *reader,
        unsigned int port_no)
{
    HRESULT hr;

    assert(reader != NULL);

    hr = aime_io_init();

    if (FAILED(hr)) {
        return hr;
    }

    sg_nfc_init(&reader->nfc, 0x00, &sg_reader_nfc_ops, reader);
    sg_led_init(&reader->led, 0x08, &sg_reader_led_ops, reader);

    InitializeCriticalSection(&reader->lock);

    uart_init(&reader->uart, port_no);
    reader->uart.written.bytes = reader->written_bytes;
    reader->uart.written.nbytes = sizeof(reader->written_bytes);
    reader->uart.readable.bytes = reader->readable_bytes;
    reader->uart.readable.nbytes = sizeof(reader->readable_bytes);

    return S_OK;
}

bool sg_reader_match_irp(const struct sg_reader *reader, const struct irp *irp)
{
    assert(reader != NULL);
    assert(irp != NULL);

    return uart_match_irp(&reader->uart, irp);
}

HRESULT sg_reader_handle_irp(struct sg_reader *reader, struct irp *irp)
{
    HRESULT hr;

    assert(reader != NULL);
    assert(irp != NULL);

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

static HRESULT sg_reader_mifare_poll(void *ctx, uint32_t *uid)
{
    return aime_io_mifare_poll(0, uid);
}

static HRESULT sg_reader_mifare_read_luid(
        void *ctx,
        uint32_t uid,
        uint8_t *luid,
        size_t luid_size)
{
    return aime_io_mifare_read_luid(0, uid, luid, luid_size);
}

static void sg_reader_led_set_color(void *ctx, uint8_t r, uint8_t g, uint8_t b)
{
    aime_io_led_set_color(0, r, g, b);
}
