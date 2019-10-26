#include <windows.h>

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "board/sg-cmd.h"
#include "board/sg-nfc.h"
#include "board/sg-nfc-cmd.h"

#include "iccard/aime.h"
#include "iccard/felica.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT sg_nfc_dispatch(
        void *ctx,
        const void *v_req,
        void *v_res);

static HRESULT sg_nfc_cmd_reset(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_res_header *res);

static HRESULT sg_nfc_cmd_get_fw_version(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_res_get_fw_version *res);

static HRESULT sg_nfc_cmd_get_hw_version(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_res_get_hw_version *res);

static HRESULT sg_nfc_cmd_poll(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_res_poll *res);

static HRESULT sg_nfc_poll_aime(
        struct sg_nfc *nfc,
        struct sg_nfc_poll_mifare *mifare);

static HRESULT sg_nfc_poll_felica(
        struct sg_nfc *nfc,
        struct sg_nfc_poll_felica *felica);

static HRESULT sg_nfc_cmd_mifare_read_block(
        struct sg_nfc *nfc,
        const struct sg_nfc_req_mifare_read_block *req,
        struct sg_nfc_res_mifare_read_block *res);

static HRESULT sg_nfc_cmd_felica_encap(
        struct sg_nfc *nfc,
        const struct sg_nfc_req_felica_encap *req,
        struct sg_nfc_res_felica_encap *res);

static HRESULT sg_nfc_cmd_dummy(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_res_header *res);

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
        struct iobuf *res_frame,
        const void *req_bytes,
        size_t req_nbytes)
{
    assert(nfc != NULL);
    assert(res_frame != NULL);
    assert(req_bytes != NULL);

    sg_req_transact(res_frame, req_bytes, req_nbytes, sg_nfc_dispatch, nfc);
}

static HRESULT sg_nfc_dispatch(
        void *ctx,
        const void *v_req,
        void *v_res)
{
    struct sg_nfc *nfc;
    const union sg_nfc_req_any *req;
    union sg_nfc_res_any *res;

    nfc = ctx;
    req = v_req;
    res = v_res;

    if (req->simple.hdr.addr != nfc->addr) {
        /* Not addressed to us, don't send a response */
        return S_FALSE;
    }

    switch (req->simple.hdr.cmd) {
    case SG_NFC_CMD_RESET:
        return sg_nfc_cmd_reset(nfc, &req->simple, &res->simple);

    case SG_NFC_CMD_GET_FW_VERSION:
        return sg_nfc_cmd_get_fw_version(
                nfc,
                &req->simple,
                &res->get_fw_version);

    case SG_NFC_CMD_GET_HW_VERSION:
        return sg_nfc_cmd_get_hw_version(
                nfc,
                &req->simple,
                &res->get_hw_version);

    case SG_NFC_CMD_POLL:
        return sg_nfc_cmd_poll(
                nfc,
                &req->simple,
                &res->poll);

    case SG_NFC_CMD_MIFARE_READ_BLOCK:
        return sg_nfc_cmd_mifare_read_block(
                nfc,
                &req->mifare_read_block,
                &res->mifare_read_block);

    case SG_NFC_CMD_FELICA_ENCAP:
        return sg_nfc_cmd_felica_encap(
                nfc,
                &req->felica_encap,
                &res->felica_encap);

    case SG_NFC_CMD_MIFARE_AUTHENTICATE:
    case SG_NFC_CMD_MIFARE_SELECT_TAG:
    case SG_NFC_CMD_MIFARE_SET_KEY_AIME:
    case SG_NFC_CMD_MIFARE_SET_KEY_BANA:
    case SG_NFC_CMD_RADIO_ON:
    case SG_NFC_CMD_RADIO_OFF:
        return sg_nfc_cmd_dummy(nfc, &req->simple, &res->simple);

    default:
        sg_nfc_dprintf(nfc, "Unimpl command %02x\n", req->simple.hdr.cmd);

        return E_NOTIMPL;
    }
}

static HRESULT sg_nfc_cmd_reset(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_res_header *res)
{
    sg_nfc_dprintf(nfc, "Reset\n");
    sg_res_init(res, req, 0);
    res->status = 3;

    return S_OK;
}

static HRESULT sg_nfc_cmd_get_fw_version(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_res_get_fw_version *res)
{
    /* Dest version is not NUL terminated, this is intentional */
    sg_res_init(&res->res, req, sizeof(res->version));
    memcpy(res->version, "TN32MSEC003S F/W Ver1.2E", sizeof(res->version));

    return S_OK;
}

static HRESULT sg_nfc_cmd_get_hw_version(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_res_get_hw_version *res)
{
    /* Dest version is not NUL terminated, this is intentional */
    sg_res_init(&res->res, req, sizeof(res->version));
    memcpy(res->version, "TN32MSEC003S H/W Ver3.0J", sizeof(res->version));

    return S_OK;
}

static HRESULT sg_nfc_cmd_poll(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_nfc_res_poll *res)
{
    struct sg_nfc_poll_mifare mifare;
    struct sg_nfc_poll_felica felica;
    HRESULT hr;

    hr = nfc->ops->poll(nfc->ops_ctx);

    if (FAILED(hr)) {
        return hr;
    }

    hr = sg_nfc_poll_felica(nfc, &felica);

    if (SUCCEEDED(hr) && hr != S_FALSE) {
        sg_res_init(&res->res, req, 1 + sizeof(felica));
        memcpy(res->payload, &felica, sizeof(felica));
        res->count = 1;

        return S_OK;
    }

    hr = sg_nfc_poll_aime(nfc, &mifare);

    if (SUCCEEDED(hr) && hr != S_FALSE) {
        sg_res_init(&res->res, req, 1 + sizeof(mifare));
        memcpy(res->payload, &mifare, sizeof(mifare));
        res->count = 1;

        return S_OK;
    }

    sg_res_init(&res->res, req, 1);
    res->count = 0;

    return S_OK;
}

static HRESULT sg_nfc_poll_aime(
        struct sg_nfc *nfc,
        struct sg_nfc_poll_mifare *mifare)
{
    uint8_t luid[10];
    HRESULT hr;

    /* Call backend */

    if (nfc->ops->get_aime_id != NULL) {
        hr = nfc->ops->get_aime_id(nfc->ops_ctx, luid, sizeof(luid));
    } else {
        hr = S_FALSE;
    }

    if (FAILED(hr) || hr == S_FALSE) {
        return hr;
    }

    sg_nfc_dprintf(nfc, "AiMe card is present\n");

    /* Construct response (use an arbitrary UID) */

    mifare->type = 0x10;
    mifare->id_len = sizeof(mifare->uid);
    mifare->uid = _byteswap_ulong(0x01020304);

    /* Initialize MIFARE IC emulator */

    hr = aime_card_populate(&nfc->mifare, luid, sizeof(luid));

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT sg_nfc_poll_felica(
        struct sg_nfc *nfc,
        struct sg_nfc_poll_felica *felica)
{
    uint64_t IDm;
    HRESULT hr;

    /* Call backend */

    if (nfc->ops->get_felica_id != NULL) {
        hr = nfc->ops->get_felica_id(nfc->ops_ctx, &IDm);
    } else {
        hr = S_FALSE;
    }

    if (FAILED(hr) || hr == S_FALSE) {
        return hr;
    }

    sg_nfc_dprintf(nfc, "FeliCa card is present\n");

    /* Construct poll response */

    felica->type = 0x20;
    felica->id_len = sizeof(felica->IDm) + sizeof(felica->PMm);
    felica->IDm = _byteswap_uint64(IDm);
    felica->PMm = _byteswap_uint64(felica_get_generic_PMm());

    /* Initialize FeliCa IC emulator */

    nfc->felica.IDm = IDm;
    nfc->felica.PMm = felica_get_generic_PMm();
    nfc->felica.system_code = 0x0000;

    return S_OK;
}

static HRESULT sg_nfc_cmd_mifare_read_block(
        struct sg_nfc *nfc,
        const struct sg_nfc_req_mifare_read_block *req,
        struct sg_nfc_res_mifare_read_block *res)
{
    uint32_t uid;

    if (req->req.payload_len != sizeof(req->payload)) {
        sg_nfc_dprintf(nfc, "%s: Payload size is incorrect\n", __func__);

        return E_FAIL;
    }

    uid = _byteswap_ulong(req->payload.uid);

    sg_nfc_dprintf(nfc, "Read uid %08x block %i\n", uid, req->payload.block_no);

    if (req->payload.block_no > 3) {
        sg_nfc_dprintf(nfc, "MIFARE block number out of range\n");

        return E_FAIL;
    }

    sg_res_init(&res->res, &req->req, sizeof(res->block));

    memcpy( res->block,
            nfc->mifare.sectors[0].blocks[req->payload.block_no].bytes,
            sizeof(res->block));

    return S_OK;
}

static HRESULT sg_nfc_cmd_felica_encap(
        struct sg_nfc *nfc,
        const struct sg_nfc_req_felica_encap *req,
        struct sg_nfc_res_felica_encap *res)
{
    struct const_iobuf f_req;
    struct iobuf f_res;
    HRESULT hr;

    /* First byte of encapsulated request and response is a length byte
       (inclusive of itself). The FeliCa emulator expects its caller to handle
       that length byte on its behalf (we adopt the convention that the length
       byte is part of the FeliCa protocol's framing layer). */

    if (req->req.payload_len != 8 + req->payload[0]) {
        sg_nfc_dprintf(
                nfc,
                "FeliCa encap payload length mismatch: sg %i != felica %i + 8",
                req->req.payload_len,
                req->payload[0]);

        return E_FAIL;
    }

    f_req.bytes = req->payload;
    f_req.nbytes = req->payload[0];
    f_req.pos = 1;

    f_res.bytes = res->payload;
    f_res.nbytes = sizeof(res->payload);
    f_res.pos = 1;

#if 0
    dprintf("FELICA OUTBOUND:\n");
    dump_const_iobuf(&f_req);
#endif

    hr = felica_transact(&nfc->felica, &f_req, &f_res);

    if (FAILED(hr)) {
        return hr;
    }

    sg_res_init(&res->res, &req->req, f_res.pos);
    res->payload[0] = f_res.pos;

#if 0
    dprintf("FELICA INBOUND:\n");
    dump_iobuf(&f_res);
#endif

    return S_OK;
}

static HRESULT sg_nfc_cmd_dummy(
        struct sg_nfc *nfc,
        const struct sg_req_header *req,
        struct sg_res_header *res)
{
    sg_res_init(res, req, 0);

    return S_OK;
}
