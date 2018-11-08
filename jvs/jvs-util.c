#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

#include "jvs/jvs-frame.h"
#include "jvs/jvs-util.h"

#include "util/dprintf.h"

typedef HRESULT (*jvs_dispatch_fn_t)(
        void *ctx,
        struct const_iobuf *req,
        struct iobuf *resp);

void jvs_crack_request(
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp,
        uint8_t jvs_addr,
        jvs_dispatch_fn_t dispatch_fn,
        void *dispatch_ctx)
{
    uint8_t req_bytes[128];
    uint8_t resp_bytes[128];
    struct iobuf decode;
    struct iobuf encode;
    struct const_iobuf segments;
    HRESULT hr;

    assert(bytes != NULL);
    assert(resp != NULL);
    assert(jvs_addr != 0x00 && (jvs_addr < 0x20 || jvs_addr == 0xFF));
    assert(dispatch_fn != NULL);

    decode.bytes = req_bytes;
    decode.nbytes = sizeof(req_bytes);
    decode.pos = 0;

    hr = jvs_frame_decode(&decode, bytes, nbytes);

    if (FAILED(hr)) {
        return;
    }

#if 0
    dprintf("Decoded request:\n");
    dump_iobuf(&decode);
#endif

    if (req_bytes[0] != jvs_addr && req_bytes[0] != 0xFF) {
        return;
    }

    iobuf_flip(&segments, &decode);
    segments.pos = 2;

    encode.bytes = resp_bytes;
    encode.nbytes = sizeof(resp_bytes);
    encode.pos = 3;

    /* +1: Don't try to dispatch the trailing checksum byte */

    hr = S_OK; /* I guess an empty request packet is technically valid? */

    while (segments.pos + 1 < segments.nbytes) {
        hr = dispatch_fn(dispatch_ctx, &segments, &encode);

        if (FAILED(hr)) {
            break;
        }
    }

    if (FAILED(hr)) {
        /* Send an error in the overall status byte */
        encode.pos = 3;

        resp_bytes[0] = 0x00;   /* Dest addr (master) */
        resp_bytes[1] = 0x02;   /* Payload len: Status byte, checksum byte */

        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            resp_bytes[2] = 0x04;   /* Status: "Overflow" */
        } else {
            resp_bytes[2] = 0x02;   /* Status: Encoutered unsupported command */
        }
    } else if (encode.pos == 3) {
        /* Probably a reset, don't emit a response frame with empty payload */
        return;
    } else {
        /* Send success response */
        resp_bytes[0] = 0x00;   /* Dest addr (master) */
        resp_bytes[1] = encode.pos - 2 + 1; /* -2 header +1 checksum */
        resp_bytes[2] = 0x01;   /* Status: Success */
    }

#if 0
    dprintf("Encoding response:\n");
    dump_iobuf(&encode);
#endif

    hr = jvs_frame_encode(resp, encode.bytes, encode.pos);

    if (FAILED(hr)) {
        dprintf("JVS Node: Response encode error: %x\n", (int) hr);
    }
}
