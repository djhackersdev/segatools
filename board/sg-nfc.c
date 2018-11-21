#include <windows.h>

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "board/sg-cmd.h"
#include "board/sg-nfc.h"
#include "board/sg-nfc-cmd.h"

#include "util/dprintf.h"

static HRESULT sg_nfc_dispatch(
        void *ctx,
        const void *v_req,
        void *v_resp);

static HRESULT sg_nfc_cmd_reset(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_resp_header *resp);

static HRESULT sg_nfc_cmd_get_fw_version(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_resp_get_fw_version *resp);

static HRESULT sg_nfc_cmd_get_hw_version(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_resp_get_hw_version *resp);

static HRESULT sg_nfc_cmd_mifare_poll(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_resp_mifare_poll *resp);

static HRESULT sg_nfc_cmd_mifare_read_block(
        const struct sg_nfc *nfc,
        const struct sg_nfc_req_mifare_read_block *req,
        struct sg_nfc_resp_mifare_read_block *resp);

static HRESULT sg_nfc_mifare_read_block_1(
        const struct sg_nfc *nfc,
        uint8_t *block);

static HRESULT sg_nfc_mifare_read_block_2(
        const struct sg_nfc *nfc,
        uint8_t *block);

static HRESULT sg_nfc_cmd_dummy(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_resp_header *resp);

static const uint8_t sg_nfc_block_1[] = {
    'S',  'B',  'D',  'T',  0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0xC6, 0x22,
};

void sg_nfc_init(
        struct sg_nfc *nfc,
        uint8_t addr,
        const struct sg_nfc_ops *ops,
        void *ops_ctx)
{
    assert(nfc != NULL);
    assert(ops != NULL);

    nfc->ops = ops;
    nfc->ops_ctx = ops_ctx;
    nfc->addr = addr;
}

#ifdef NDEBUG
#define sg_nfc_dprintfv(nfc, fmt, ap)
#define sg_nfc_dprintf(nfc, fmt, ...)
#else
static void sg_nfc_dprintfv(
        const struct sg_nfc *nfc,
        const char *fmt,
        va_list ap)
{
    dprintf("NFC %02x: ", nfc->addr);
    dprintfv(fmt, ap);
}

static void sg_nfc_dprintf(
        const struct sg_nfc *nfc,
        const char *fmt,
        ...)
{
    va_list ap;

    va_start(ap, fmt);
    sg_nfc_dprintfv(nfc, fmt, ap);
    va_end(ap);
}
#endif

void sg_nfc_transact(
        struct sg_nfc *nfc,
        struct iobuf *resp_frame,
        const void *req_bytes,
        size_t req_nbytes)
{
    assert(nfc != NULL);
    assert(resp_frame != NULL);
    assert(req_bytes != NULL);

    sg_req_transact(resp_frame, req_bytes, req_nbytes, sg_nfc_dispatch, nfc);
}

static HRESULT sg_nfc_dispatch(
        void *ctx,
        const void *v_req,
        void *v_resp)
{
    const struct sg_nfc *nfc;
    const union sg_nfc_req_any *req;
    union sg_nfc_resp_any *resp;

    nfc = ctx;
    req = v_req;
    resp = v_resp;

    if (req->simple.hdr.addr != nfc->addr) {
        /* Not addressed to us, don't send a response */
        return S_FALSE;
    }

    switch (req->simple.hdr.cmd) {
    case SG_NFC_CMD_RESET:
        return sg_nfc_cmd_reset(nfc, &req->simple, &resp->simple);

    case SG_NFC_CMD_GET_FW_VERSION:
        return sg_nfc_cmd_get_fw_version(
                nfc,
                &req->simple,
                &resp->get_fw_version);

    case SG_NFC_CMD_GET_HW_VERSION:
        return sg_nfc_cmd_get_hw_version(
                nfc,
                &req->simple,
                &resp->get_hw_version);

    case SG_NFC_CMD_MIFARE_POLL:
        return sg_nfc_cmd_mifare_poll(
                nfc,
                &req->simple,
                &resp->mifare_poll);

    case SG_NFC_CMD_MIFARE_READ_BLOCK:
        return sg_nfc_cmd_mifare_read_block(
                nfc,
                &req->mifare_read_block,
                &resp->mifare_read_block);

    case SG_NFC_CMD_40_POLL:
    case SG_NFC_CMD_41_POLL:
    case SG_NFC_CMD_MIFARE_SET_KEY:
    case SG_NFC_CMD_MIFARE_SELECT_TAG:
    case SG_NFC_CMD_MIFARE_50:
    case SG_NFC_CMD_MIFARE_55:
        return sg_nfc_cmd_dummy(nfc, &req->simple, &resp->simple);

    default:
        sg_nfc_dprintf(nfc, "Unimpl command %02x\n", req->simple.hdr.cmd);

        return E_NOTIMPL;
    }
}

static HRESULT sg_nfc_cmd_reset(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_resp_header *resp)
{
    sg_nfc_dprintf(nfc, "Reset\n");
    sg_resp_init(resp, req, 0);
    resp->status = 3;

    return S_OK;
}

static HRESULT sg_nfc_cmd_get_fw_version(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_resp_get_fw_version *resp)
{
    /* Dest version is not NUL terminated, this is intentional */
    sg_resp_init(&resp->resp, req, sizeof(resp->version));
    memcpy(resp->version, "TN32MSEC003S F/W Ver1.2E", sizeof(resp->version));

    return S_OK;
}

static HRESULT sg_nfc_cmd_get_hw_version(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_resp_get_hw_version *resp)
{
    /* Dest version is not NUL terminated, this is intentional */
    sg_resp_init(&resp->resp, req, sizeof(resp->version));
    memcpy(resp->version, "TN32MSEC003S H/W Ver3.0J", sizeof(resp->version));

    return S_OK;
}

static HRESULT sg_nfc_cmd_mifare_poll(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_resp_mifare_poll *resp)
{
    HRESULT hr;

    hr = nfc->ops->mifare_poll(nfc->ops_ctx);

    if (hr == S_OK) {
        sg_nfc_dprintf(nfc, "Mifare card is present\n");

        sg_resp_init(&resp->resp, req, sizeof(resp->payload.some));
        resp->payload.some[0] = 0x01;   /* Chunk size? */
        resp->payload.some[1] = 0x10;   /* Unknown */
        resp->payload.some[2] = 0x04;   /* Chunk size? */
        resp->payload.some[3] = 0x52;   /* UID byte 0 */
        resp->payload.some[4] = 0xCC;   /* UID byte 1 */
        resp->payload.some[5] = 0x55;   /* UID byte 2 */
        resp->payload.some[6] = 0x25;   /* UID byte 3 */

        return S_OK;
    } else if (hr == S_FALSE) {
        sg_resp_init(&resp->resp, req, sizeof(resp->payload.none));
        resp->payload.none = 0x00;

        return S_OK;
    } else if (FAILED(hr)) {
        sg_nfc_dprintf(nfc, "nfc->ops->mifare_poll error: %x\n", (int) hr);

        return hr;
    } else {
        sg_nfc_dprintf(nfc, "nfc->ops->mifare_poll bad return: %x\n", (int) hr);

        return E_UNEXPECTED;
    }
}

static HRESULT sg_nfc_cmd_mifare_read_block(
        const struct sg_nfc *nfc,
        const struct sg_nfc_req_mifare_read_block *req,
        struct sg_nfc_resp_mifare_read_block *resp)
{
    if (req->req.payload_len != sizeof(req->payload)) {
        sg_nfc_dprintf(nfc, "%s: Payload size is incorrect\n", __func__);

        return E_FAIL;
    }

    sg_nfc_dprintf(nfc, "Read block %i\n", req->payload.block_no);

    sg_resp_init(&resp->resp, &req->req, sizeof(resp->block));

    switch (req->payload.block_no) {
    case 1:
        return sg_nfc_mifare_read_block_1(nfc, resp->block);

    case 2:
        return sg_nfc_mifare_read_block_2(nfc, resp->block);

    case 0:
    case 3:
        sg_nfc_dprintf(
                nfc,
                "Block %i access not implemented\n",
                req->payload.block_no);

        return E_NOTIMPL;

    default:
        sg_nfc_dprintf(
                nfc,
                "Read from invalid mifare block nr %i\n",
                req->payload.block_no);

        return E_INVALIDARG;
    }
}

static HRESULT sg_nfc_mifare_read_block_1(
        const struct sg_nfc *nfc,
        uint8_t *block)
{
    memcpy(block, sg_nfc_block_1, sizeof(sg_nfc_block_1));

    return S_OK;
}

static HRESULT sg_nfc_mifare_read_block_2(
        const struct sg_nfc *nfc,
        uint8_t *block)
{
    HRESULT hr;

    hr = nfc->ops->mifare_read_luid(nfc->ops_ctx, &block[6], 10);

    if (FAILED(hr)) {
        return hr;
    }

    memset(block, 0, 6);

    return S_OK;
}

static HRESULT sg_nfc_cmd_dummy(
        const struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_resp_header *resp)
{
    sg_resp_init(resp, req, 0);

    return S_OK;
}
