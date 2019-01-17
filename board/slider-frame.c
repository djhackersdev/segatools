#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board/slider-frame.h"

#include "hook/iobuf.h"

static void slider_frame_sync(struct iobuf *src);
static HRESULT slider_frame_accept(const struct iobuf *dest);
static HRESULT slider_frame_encode_byte(struct iobuf *dest, uint8_t byte);

/* Frame structure:

   [0] Sync byte (0xFF)
   [1] Command
   [2] Payload size
   ... Payload
   [n] Checksum: Negate the sum of all prior bytes (including sync byte!)

   Byte stuffing:

   0xFD is an escape byte. Un-escape the subsequent byte by adding 1. */

static void slider_frame_sync(struct iobuf *src)
{
    size_t i;

    for (i = 0 ; i < src->pos && src->bytes[i] != 0xFF ; i++);

    src->pos -= i;
    memmove(&src->bytes[0], &src->bytes[i], i);
}

static HRESULT slider_frame_accept(const struct iobuf *dest)
{
    uint8_t checksum;
    size_t i;

    if (dest->pos < 2 || dest->pos != dest->bytes[2] + 4) {
        return S_FALSE;
    }

    checksum = 0;

    for (i = 0 ; i < dest->pos ; i++) {
        checksum += dest->bytes[i];
    }

    if (checksum != 0) {
        return HRESULT_FROM_WIN32(ERROR_CRC);
    }

    return S_OK;
}

HRESULT slider_frame_decode(struct iobuf *dest, struct iobuf *src)
{
    uint8_t byte;
    bool escape;
    size_t i;
    HRESULT hr;

    assert(dest != NULL);
    assert(dest->bytes != NULL || dest->nbytes == 0);
    assert(dest->pos <= dest->nbytes);
    assert(src != NULL);
    assert(src->bytes != NULL || src->nbytes == 0);
    assert(src->pos <= src->nbytes);

    slider_frame_sync(src);

    dest->pos = 0;
    escape = false;

    for (i = 0, hr = S_FALSE ; i < src->pos && hr == S_FALSE ; i++) {
        /* Step the FSM to unstuff another byte */

        byte = src->bytes[i];

        if (dest->pos >= dest->nbytes) {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        } else if (i == 0) {
            dest->bytes[dest->pos++] = byte;
        } else if (byte == 0xFF) {
            hr = E_FAIL;
        } else if (byte == 0xFD) {
            if (escape) {
                hr = E_FAIL;
            }

            escape = true;
        } else if (escape) {
            dest->bytes[dest->pos++] = byte + 1;
            escape = false;
        } else {
            dest->bytes[dest->pos++] = byte;
        }

        /* Try to accept the packet we've built up so far */

        if (SUCCEEDED(hr)) {
            hr = slider_frame_accept(dest);
        }
    }

    /* Handle FSM terminal state */

    if (hr != S_FALSE) {
        /* Frame was either accepted or rejected, remove it from src */
        memmove(&src->bytes[0], &src->bytes[i], src->pos - i);
        src->pos -= i;
    }

    return hr;
}

HRESULT slider_frame_encode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes)
{
    const uint8_t *src;
    uint8_t checksum;
    uint8_t byte;
    size_t i;
    HRESULT hr;

    assert(dest != NULL);
    assert(dest->bytes != NULL || dest->nbytes == 0);
    assert(dest->pos <= dest->nbytes);
    assert(ptr != NULL);

    src = ptr;

    assert(nbytes >= 2 && src[0] == 0xFF && src[2] + 3 == nbytes);

    if (dest->pos >= dest->nbytes) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dest->bytes[dest->pos++] = 0xFF;
    checksum = 0xFF;

    for (i = 1 ; i < nbytes ; i++) {
        byte = src[i];
        checksum += byte;

        hr = slider_frame_encode_byte(dest, byte);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return slider_frame_encode_byte(dest, -checksum);
}

static HRESULT slider_frame_encode_byte(struct iobuf *dest, uint8_t byte)
{
    if (byte == 0xFF || byte == 0xFD) {
        if (dest->pos + 2 > dest->nbytes) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        dest->bytes[dest->pos++] = 0xFD;
        dest->bytes[dest->pos++] = byte - 1;
    } else {
        if (dest->pos + 1 > dest->nbytes) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        dest->bytes[dest->pos++] = byte;
    }

    return S_OK;
}
