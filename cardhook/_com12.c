#include <windows.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "board/sg-led.h"
#include "board/sg-nfc.h"

#include "cardhook/_com12.h"

#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/crc.h"
#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT com12_handle_irp(struct irp *irp);
static HRESULT com12_handle_irp_locked(struct irp *irp);

static HRESULT com12_mifare_poll(void *ctx, uint32_t *uid);
static HRESULT com12_mifare_read_luid(
        void *ctx,
        uint32_t uid,
        uint8_t *luid,
        size_t nbytes);
static HRESULT com12_led_set_color(void *ctx, uint8_t r, uint8_t g, uint8_t b);

static const struct sg_nfc_ops com12_nfc_ops = {
    .mifare_poll        = com12_mifare_poll,
    .mifare_read_luid   = com12_mifare_read_luid,
};

static const struct sg_led_ops com12_led_ops = {
    .set_color          = com12_led_set_color,
};

static struct sg_nfc com12_nfc;
static struct sg_led com12_led;

static const char com12_aime_path[] = "DEVICE\\aime.txt";

static CRITICAL_SECTION com12_lock;
static struct uart com12_uart;
static uint8_t com12_written_bytes[520];
static uint8_t com12_readable_bytes[520];
static uint8_t com12_aime_luid[10];

void com12_hook_init(void)
{
    sg_nfc_init(&com12_nfc, 0x00, &com12_nfc_ops, NULL);
    sg_led_init(&com12_led, 0x08, &com12_led_ops, NULL);

    InitializeCriticalSection(&com12_lock);

    uart_init(&com12_uart, 12);
    com12_uart.written.bytes = com12_written_bytes;
    com12_uart.written.nbytes = sizeof(com12_written_bytes);
    com12_uart.readable.bytes = com12_readable_bytes;
    com12_uart.readable.nbytes = sizeof(com12_readable_bytes);

    iohook_push_handler(com12_handle_irp);
}

static HRESULT com12_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&com12_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&com12_lock);
    hr = com12_handle_irp_locked(irp);
    LeaveCriticalSection(&com12_lock);

    return hr;
}

static HRESULT com12_handle_irp_locked(struct irp *irp)
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
        dump_iobuf(&com12_uart.readable);
    }
#endif

    hr = uart_handle_irp(&com12_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    sg_nfc_transact(
            &com12_nfc,
            &com12_uart.readable,
            com12_uart.written.bytes,
            com12_uart.written.pos);

    sg_led_transact(
            &com12_led,
            &com12_uart.readable,
            com12_uart.written.bytes,
            com12_uart.written.pos);

    com12_uart.written.pos = 0;

    return hr;
}

static HRESULT com12_mifare_poll(void *ctx, uint32_t *uid)
{
    HRESULT hr;
    FILE *f;
    size_t i;
    int byte;
    int r;

    hr = S_FALSE;
    f = NULL;

    if (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {
        goto end;
    }

    f = fopen(com12_aime_path, "r");

    if (f == NULL) {
        dprintf("Aime reader: Failed to open %s\n", com12_aime_path);

        goto end;
    }

    for (i = 0 ; i < sizeof(com12_aime_luid) ; i++) {
        r = fscanf(f, "%02x ", &byte);

        if (r != 1) {
            dprintf("Aime reader: fscanf[%i] failed: %i\n", (int) i, r);

            goto end;
        }

        com12_aime_luid[i] = byte;
    }

    /* NOTE: We are just arbitrarily using the CRC32 of the LUID here, real
       cards do not work like this! However, neither the application code nor
       the network protocol care what the UID is, it just has to be a stable
       unique identifier for over-the-air NFC communications. */

    *uid = crc32(com12_aime_luid, sizeof(com12_aime_luid), 0);

    hr = S_OK;

end:
    if (f != NULL) {
        fclose(f);
    }

    return hr;
}

static HRESULT com12_mifare_read_luid(
        void *ctx,
        uint32_t uid,
        uint8_t *luid,
        size_t nbytes)
{
    assert(luid != NULL);
    assert(nbytes == sizeof(com12_aime_luid));

    memcpy(luid, com12_aime_luid, nbytes);

    return S_OK;
}

static HRESULT com12_led_set_color(void *ctx, uint8_t r, uint8_t g, uint8_t b)
{
    return S_OK;
}
