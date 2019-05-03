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

static HRESULT sg_reader_handle_irp(struct irp *irp);
static HRESULT sg_reader_handle_irp_locked(struct irp *irp);
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

static CRITICAL_SECTION sg_reader_lock;
static struct uart sg_reader_uart;
static uint8_t sg_reader_written_bytes[520];
static uint8_t sg_reader_readable_bytes[520];
static struct sg_nfc sg_reader_nfc;
static struct sg_led sg_reader_led;

HRESULT sg_reader_hook_init(unsigned int port_no)
{
    HRESULT hr;

    hr = aime_io_init();

    if (FAILED(hr)) {
        return hr;
    }

    sg_nfc_init(&sg_reader_nfc, 0x00, &sg_reader_nfc_ops, NULL);
    sg_led_init(&sg_reader_led, 0x08, &sg_reader_led_ops, NULL);

    InitializeCriticalSection(&sg_reader_lock);

    uart_init(&sg_reader_uart, port_no);
    sg_reader_uart.written.bytes = sg_reader_written_bytes;
    sg_reader_uart.written.nbytes = sizeof(sg_reader_written_bytes);
    sg_reader_uart.readable.bytes = sg_reader_readable_bytes;
    sg_reader_uart.readable.nbytes = sizeof(sg_reader_readable_bytes);

    return iohook_push_handler(sg_reader_handle_irp);
}

static HRESULT sg_reader_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&sg_reader_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&sg_reader_lock);
    hr = sg_reader_handle_irp_locked(irp);
    LeaveCriticalSection(&sg_reader_lock);

    return hr;
}

static HRESULT sg_reader_handle_irp_locked(struct irp *irp)
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
        dump_iobuf(&sg_reader_uart.readable);
    }
#endif

    hr = uart_handle_irp(&sg_reader_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    sg_nfc_transact(
            &sg_reader_nfc,
            &sg_reader_uart.readable,
            sg_reader_uart.written.bytes,
            sg_reader_uart.written.pos);

    sg_led_transact(
            &sg_reader_led,
            &sg_reader_uart.readable,
            sg_reader_uart.written.bytes,
            sg_reader_uart.written.pos);

    sg_reader_uart.written.pos = 0;

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
