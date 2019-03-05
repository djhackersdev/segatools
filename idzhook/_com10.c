#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "aimeio/aimeio.h"

#include "board/sg-led.h"
#include "board/sg-nfc.h"
#include "board/sg-reader.h"

#include "divahook/_com10.h"

#include "hook/iohook.h"

static HRESULT com10_handle_irp(struct irp *irp);
static HRESULT com10_mifare_poll(void *ctx, uint32_t *uid);
static HRESULT com10_mifare_read_luid(
        void *ctx,
        uint32_t uid,
        uint8_t *luid,
        size_t nbytes);
static void com10_led_set_color(void *ctx, uint8_t r, uint8_t g, uint8_t b);

static const struct sg_nfc_ops com10_nfc_ops = {
    .mifare_poll        = com10_mifare_poll,
    .mifare_read_luid   = com10_mifare_read_luid,
};

static const struct sg_led_ops com10_led_ops = {
    .set_color          = com10_led_set_color,
};

static struct sg_reader com10_reader;

HRESULT com10_hook_init(void)
{
    HRESULT hr;

    hr = aime_io_init();

    if (FAILED(hr)) {
        return hr;
    }

    sg_reader_init(&com10_reader, 10, &com10_nfc_ops, &com10_led_ops, NULL);

    return iohook_push_handler(com10_handle_irp);
}

static HRESULT com10_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    return sg_reader_handle_irp(&com10_reader, irp);
}

static HRESULT com10_mifare_poll(void *ctx, uint32_t *uid)
{
    return aime_io_mifare_poll(0, uid);
}

static HRESULT com10_mifare_read_luid(
        void *ctx,
        uint32_t uid,
        uint8_t *luid,
        size_t luid_size)
{
    return aime_io_mifare_read_luid(0, uid, luid, luid_size);
}

static void com10_led_set_color(void *ctx, uint8_t r, uint8_t g, uint8_t b)
{
    aime_io_led_set_color(0, r, g, b);
}
