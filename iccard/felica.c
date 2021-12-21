#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "hook/iobuf.h"

#include "iccard/felica.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT felica_cmd_poll(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res);

static HRESULT felica_cmd_get_system_code(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res);

static HRESULT felica_cmd_nda_a4(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res);

uint64_t felica_get_generic_PMm(void)
{
    /* A FeliCa PMm contains low-level protocol timing information for
       communicating with a particular IC card. The exact values are not
       particularly important for our purposes, so we'll just return a hard-
       coded PMm. This current value has been taken from an iPhone, emulating
       a Suica pass via Apple Wallet, which seems to be one of the few
       universally accepted FeliCa types for these games. Certain older
       Suica passes and other payment and transportation cards
       do not seem to be supported anymore. */

    return 0x01168B868FBECBFFULL;
}

HRESULT felica_transact(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res)
{
    uint64_t IDm;
    uint8_t code;
    HRESULT hr;

    assert(f != NULL);
    assert(req != NULL);
    assert(res != NULL);

    hr = iobuf_read_8(req, &code);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_write_8(res, code + 1);

    if (FAILED(hr)) {
        return hr;
    }

    if (code != FELICA_CMD_POLL) {
        hr = iobuf_read_be64(req, &IDm);

        if (FAILED(hr)) {
            return hr;
        }

        if (IDm != f->IDm) {
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }

        hr = iobuf_write_be64(res, IDm);

        if (FAILED(hr)) {
            return hr;
        }
    }

    switch (code) {
    case FELICA_CMD_POLL:
        return felica_cmd_poll(f, req, res);

    case FELICA_CMD_GET_SYSTEM_CODE:
        return felica_cmd_get_system_code(f, req, res);

    case FELICA_CMD_NDA_A4:
        return felica_cmd_nda_a4(f, req, res);

    default:
        dprintf("FeliCa: Unimplemented command %02x, payload:\n", code);
        dump_const_iobuf(req);

        return E_NOTIMPL;
    }
}

static HRESULT felica_cmd_poll(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res)
{
    uint16_t system_code;
    uint8_t request_code;
    uint8_t time_slot;
    HRESULT hr;

    /* Request: */

    hr = iobuf_read_be16(req, &system_code);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_read_8(req, &request_code);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_read_8(req, &time_slot);

    if (FAILED(hr)) {
        return hr;
    }

    if (system_code != 0xFFFF && system_code != f->system_code) {
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    // TODO handle other params correctly...

    /* Response: */

    hr = iobuf_write_be64(res, f->IDm);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_write_be64(res, f->PMm);

    if (FAILED(hr)) {
        return hr;
    }

    if (request_code == 0x01) {
        hr = iobuf_write_be16(res, f->system_code);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}

static HRESULT felica_cmd_get_system_code(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res)
{
    HRESULT hr;

    hr = iobuf_write_8(res, 1); /* Number of system codes */

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_write_be16(res, f->system_code);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT felica_cmd_nda_a4(
        struct felica *f,
        struct const_iobuf *req,
        struct iobuf *res)
{
    /* The specification for this command is probably only available under NDA.
       Returning what the driver seems to want. */

    return iobuf_write_8(res, 0);
}
