#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board/sg-frame.h"

#include "hook/iobuf.h"

#include "util/dprintf.h"

static HRESULT sg_frame_accept(struct iobuf *dest);
static HRESULT sg_frame_encode_byte(struct iobuf *dest, uint8_t byte);

/* Frame structure:

   [0] Sync byte (0xE0)
   [1] Frame size (including self)
   [2] Address
   [3] Sequence no
   ... Body
   [n] Checksum: Sum of all non-framing bytes

   Byte stuffing:

   0xD0 is an escape byte. Un-escape the subsequent byte by adding 1. */

static HRESULT sg_frame_accept(struct iobuf *dest)
{
    uint8_t checksum;
    size_t i;

    if (dest->pos < 1 || dest->pos != dest->bytes[0] + 1) {
        dprintf("SG Frame: Size mismatch\n");

        return S_FALSE;
    }

    checksum = 0;

    for (i = 0 ; i < dest->pos - 1 ; i++) {
        checksum += dest->bytes[i];
    }

    if (checksum != dest->bytes[dest->pos - 1]) {
        dprintf("SG Frame: Checksum mismatch\n");

        return HRESULT_FROM_WIN32(ERROR_CRC);
    }

    /* Discard checksum */
    dest->pos--;

    return S_OK;
}

HRESULT sg_frame_decode(struct iobuf *dest, const uint8_t *bytes, size_t nbytes)
{
    uint8_t byte;
    size_t i;

    assert(dest != NULL);
    assert(dest->bytes != NULL || dest->nbytes == 0);
    assert(dest->pos <= dest->nbytes);
    assert(bytes != NULL);

    if (nbytes < 1 || bytes[0] != 0xE0) {
        dprintf("SG Frame: Bad sync\n");

        return E_FAIL;
    }

    dest->pos = 0;
    i = 1;

    while (i < nbytes) {
        if (dest->pos >= dest->nbytes) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        byte = bytes[i++];

        if (byte == 0xE0) {
            dprintf("SG Frame: Unescaped sync\n");

            return E_FAIL;
        } else if (byte == 0xD0) {
            if (i >= nbytes) {
                dprintf("SG Frame: Trailing escape\n");

                return E_FAIL;
            }

            byte = bytes[i++];
            dest->bytes[dest->pos++] = byte + 1;
        } else {
            dest->bytes[dest->pos++] = byte;
        }
    }

    return sg_frame_accept(dest);
}

HRESULT sg_frame_encode(
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

    assert(nbytes != 0 && src[0] == nbytes);

    if (dest->pos >= dest->nbytes) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dest->bytes[dest->pos++] = 0xE0;
    checksum = 0;

    for (i = 0 ; i < nbytes ; i++) {
        byte = src[i];
        checksum += byte;

        hr = sg_frame_encode_byte(dest, byte);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return sg_frame_encode_byte(dest, checksum);
}

static HRESULT sg_frame_encode_byte(struct iobuf *dest, uint8_t byte)
{
    if (byte == 0xD0 || byte == 0xE0) {
        if (dest->pos + 2 > dest->nbytes) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        dest->bytes[dest->pos++] = 0xD0;
        dest->bytes[dest->pos++] = byte - 1;
    } else {
        if (dest->pos + 1 > dest->nbytes) {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        dest->bytes[dest->pos++] = byte;
    }

    return S_OK;
}
