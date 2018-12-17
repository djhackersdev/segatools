#include <assert.h>

#include "board/sg-cmd.h"
#include "board/sg-frame.h"

#include "hook/iobuf.h"

#include "util/dprintf.h"

union sg_req_any {
    struct sg_req_header req;
    uint8_t bytes[256];
};

union sg_resp_any {
    struct sg_resp_header resp;
    uint8_t bytes[256];
};

static HRESULT sg_req_validate(const void *ptr, size_t nbytes);

static void sg_resp_error(
        struct sg_resp_header *resp,
        const struct sg_req_header *req);

static HRESULT sg_req_validate(const void *ptr, size_t nbytes)
{
    const struct sg_req_header *req;
    size_t payload_len;

    assert(ptr != NULL);

    if (nbytes < sizeof(*req)) {
        dprintf("SG Cmd: Request header truncated\n");

        return E_FAIL;
    }

    req = ptr;

    if (req->hdr.frame_len != nbytes) {
        dprintf("SG Cmd: Frame length mismatch: got %i exp %i\n",
                req->hdr.frame_len,
                (int) nbytes);

        return E_FAIL;
    }

    payload_len = req->hdr.frame_len - sizeof(*req);

    if (req->payload_len != payload_len) {
        dprintf("SG Cmd: Payload length mismatch: got %i exp %i\n",
                req->payload_len,
                (int) payload_len);

        return E_FAIL;
    }

    return S_OK;
}

void sg_req_transact(
        struct iobuf *resp_frame,
        const uint8_t *req_bytes,
        size_t req_nbytes,
        sg_dispatch_fn_t dispatch,
        void *ctx)
{
    struct iobuf req_span;
    union sg_req_any req;
    union sg_resp_any resp;
    HRESULT hr;

    assert(resp_frame != NULL);
    assert(req_bytes != NULL);
    assert(dispatch != NULL);

    req_span.bytes = req.bytes;
    req_span.nbytes = sizeof(req.bytes);
    req_span.pos = 0;

    hr = sg_frame_decode(&req_span, req_bytes, req_nbytes);

    if (FAILED(hr)) {
        return;
    }

    hr = sg_req_validate(req.bytes, req_span.pos);

    if (FAILED(hr)) {
        return;
    }

    hr = dispatch(ctx, &req, &resp);

    if (hr != S_FALSE) {
        if (FAILED(hr)) {
            sg_resp_error(&resp.resp, &req.req);
        }

        sg_frame_encode(resp_frame, resp.bytes, resp.resp.hdr.frame_len);
    }
}

void sg_resp_init(
        struct sg_resp_header *resp,
        const struct sg_req_header *req,
        size_t payload_len)
{
    assert(resp != NULL);
    assert(req != NULL);

    resp->hdr.frame_len = sizeof(*resp) + payload_len;
    resp->hdr.addr = req->hdr.addr;
    resp->hdr.seq_no = req->hdr.seq_no;
    resp->hdr.cmd = req->hdr.cmd;
    resp->status = 0;
    resp->payload_len = payload_len;
}

static void sg_resp_error(
        struct sg_resp_header *resp,
        const struct sg_req_header *req)
{
    assert(resp != NULL);
    assert(req != NULL);

    resp->hdr.frame_len = sizeof(*resp);
    resp->hdr.addr = req->hdr.addr;
    resp->hdr.seq_no = req->hdr.seq_no;
    resp->hdr.cmd = req->hdr.cmd;
    resp->status = 1;
    resp->payload_len = 0;
}
