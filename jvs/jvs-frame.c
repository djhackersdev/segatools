#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

#include "jvs/jvs-frame.h"

#include "util/dprintf.h"

static HRESULT jvs_frame_check_sum(const struct iobuf *dest);
static HRESULT jvs_frame_encode_byte(struct iobuf *dest, uint8_t byte);

/* Deals in whole frames only for simplicity's sake, since that's all we need
   to emulate the Nu's kernel driver interface. This could of course be
   extended later for other arcade hardware platforms. */

HRESULT jvs_frame_decode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes)
{
    const uint8_t *bytes;
    bool escape;
    size_t i;

    assert(dest != NULL);
    assert(ptr != NULL);

    bytes = ptr;

    if (nbytes == 0) {
        dprintf("JVS Frame: Empty frame\n");

        return E_FAIL;
    }

    if (bytes[0] != 0xE0) {
        dprintf("JVS Frame: Sync byte was expected\n");

        return E_FAIL;
    }

    escape = false;

    for (i = 1 ; i < nbytes ; i++) {
        if (bytes[i] == 0xE0) {
            dprintf("JVS Frame: Unexpected sync byte\n");

            return E_FAIL;
        } else if (bytes[i] == 0xD0) {
            if (escape) {
                dprintf("JVS Frame: Escaping fault\n");

                return E_FAIL;
            }

            escape = true;
        } else {
            if (dest->pos >= dest->nbytes) {
                return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }

            if (escape) {
                escape = false;
                dest->bytes[dest->pos++] = bytes[i] + 1;
            } else {
                dest->bytes[dest->pos++] = bytes[i];
            }
        }
    }

    return jvs_frame_check_sum(dest);
}

static HRESULT jvs_frame_check_sum(const struct iobuf *dest)
{
    uint8_t checksum;
    size_t i;

    checksum = 0;

    for (i = 0 ; i < dest->pos - 1 ; i++) {
        checksum += dest->bytes[i];
    }

    if (checksum != dest->bytes[dest->pos - 1]) {
        dprintf("JVS Frame: Checksum failure\n");

        return HRESULT_FROM_WIN32(ERROR_CRC);
    }

    return S_OK;
}

HRESULT jvs_frame_encode(
        struct iobuf *dest,
        const void *ptr,
        size_t nbytes)
{
    const uint8_t *bytes;
    uint8_t checksum;
    size_t i;
    HRESULT hr;

    assert(dest != NULL);
    assert(ptr != NULL);

    bytes = ptr;
    checksum = 0;

    if (dest->pos + 1 > dest->nbytes) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dest->bytes[dest->pos++] = 0xE0;

    for (i = 0 ; i < nbytes ; i++) {
        hr = jvs_frame_encode_byte(dest, bytes[i]);

        if (FAILED(hr)) {
            return hr;
        }

        checksum += bytes[i];
    }

    return jvs_frame_encode_byte(dest, checksum);
}

static HRESULT jvs_frame_encode_byte(struct iobuf *dest, uint8_t byte)
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
