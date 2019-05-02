/*
   Sega "Type 3" JVS I/O emulator

   Credits:

   Protocol docs:
   https://github.com/TheOnlyJoey/openjvs/wiki/ (a/o October 2018)

   Capability dumps:
   https://wiki.arcadeotaku.com/w/JVS#Sega_837-14572 (a/o October 2018)
*/

#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board/io3.h"

#include "jvs/jvs-bus.h"
#include "jvs/jvs-cmd.h"
#include "jvs/jvs-util.h"

#include "util/dprintf.h"
#include "util/dump.h"

static void io3_transact(
        struct jvs_node *node,
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp);

static bool io3_sense(struct jvs_node *node);

static HRESULT io3_cmd(
        void *ctx,
        struct const_iobuf *req,
        struct iobuf *resp);

static HRESULT io3_cmd_read_id(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_get_cmd_version(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_get_jvs_version(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_get_comm_version(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_get_features(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_read_switches(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_read_coin(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_read_analogs(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_write_gpio(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static HRESULT io3_cmd_reset(struct io3 *io3, struct const_iobuf *buf);

static HRESULT io3_cmd_assign_addr(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf);

static const uint8_t io3_ident[] =
        "SEGA CORPORATION;I/O BD JVS;837-14572;Ver1.00;2005/10";

static uint8_t io3_features[] = {
    /* Feature : 0x01 : Players and switches
       Param1  :    2 : Number of players
       Param2  :   14 : Number of switches per player
       Param3  :    0 : N/A */

    0x01, 2, 14, 0,

    /* Feature : 0x02 : Coin slots
       Param1  :    2 : Number of coin slots
       Param2  :    0 : N/A
       Param3  :    0 : N/A */

    0x02, 2, 0, 0,

    /* Feature : 0x03 : Analog inputs
       Param1  :    8 : Number of ADC channels
       Param2  :   10 : Effective bits of resolution per ADC
       Param3  :    0 : N/A */

    0x03, 8, 10, 0,

    /* Feature : 0x12 : GPIO outputs
       Param1  :    3 : Number of ports (8 bits per port)
       Param2  :    0 : N/A
       Param3  :    0 : N/A */

    0x12, 20, 0, 0,

    /* Feature : 0x00 : End of capabilities */

    0x00,
};

void io3_init(
        struct io3 *io3,
        struct jvs_node *next,
        const struct io3_ops *ops,
        void *ops_ctx)
{
    assert(io3 != NULL);
    assert(ops != NULL);

    io3->jvs.next = next;
    io3->jvs.transact = io3_transact;
    io3->jvs.sense = io3_sense;
    io3->addr = 0xFF;
    io3->ops = ops;
    io3->ops_ctx = ops_ctx;
}

static void io3_transact(
        struct jvs_node *node,
        const void *bytes,
        size_t nbytes,
        struct iobuf *resp)
{
    struct io3 *io3;

    assert(node != NULL);
    assert(bytes != NULL);
    assert(resp != NULL);

    io3 = CONTAINING_RECORD(node, struct io3, jvs);

    jvs_crack_request(bytes, nbytes, resp, io3->addr, io3_cmd, io3);
}

static bool io3_sense(struct jvs_node *node)
{
    struct io3 *io3;

    assert(node != NULL);

    io3 = CONTAINING_RECORD(node, struct io3, jvs);

    return io3->addr == 0xFF;
}

static HRESULT io3_cmd(
        void *ctx,
        struct const_iobuf *req,
        struct iobuf *resp)
{
    struct io3 *io3;

    io3 = ctx;

    switch (req->bytes[req->pos]) {
    case JVS_CMD_READ_ID:
        return io3_cmd_read_id(io3, req, resp);

    case JVS_CMD_GET_CMD_VERSION:
        return io3_cmd_get_cmd_version(io3, req, resp);

    case JVS_CMD_GET_JVS_VERSION:
        return io3_cmd_get_jvs_version(io3, req, resp);

    case JVS_CMD_GET_COMM_VERSION:
        return io3_cmd_get_comm_version(io3, req, resp);

    case JVS_CMD_GET_FEATURES:
        return io3_cmd_get_features(io3, req, resp);

    case JVS_CMD_READ_SWITCHES:
        return io3_cmd_read_switches(io3, req, resp);

    case JVS_CMD_READ_COIN:
        return io3_cmd_read_coin(io3, req, resp);

    case JVS_CMD_READ_ANALOGS:
        return io3_cmd_read_analogs(io3, req, resp);

    case JVS_CMD_WRITE_GPIO:
        return io3_cmd_write_gpio(io3, req, resp);

    case JVS_CMD_RESET:
        return io3_cmd_reset(io3, req);

    case JVS_CMD_ASSIGN_ADDR:
        return io3_cmd_assign_addr(io3, req, resp);

    default:
        dprintf("JVS I/O: Node %02x: Unhandled command byte %02x\n",
                io3->addr,
                req->bytes[req->pos]);

        return E_NOTIMPL;
    }
}

static HRESULT io3_cmd_read_id(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Read ID\n");

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write the identification string. The NUL terminator at the end of this C
       string is also sent, and it naturally terminates the response chunk. */

    return iobuf_write(resp_buf, io3_ident, sizeof(io3_ident));
}

static HRESULT io3_cmd_get_cmd_version(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    uint8_t resp[2];
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get command format version\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x13; /* Command format version BCD */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT io3_cmd_get_jvs_version(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    uint8_t resp[2];
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get JVS version\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x20; /* JVS version BCD */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT io3_cmd_get_comm_version(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    uint8_t resp[2];
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get communication version\n");
    resp[0] = 0x01; /* Report byte */
    resp[1] = 0x10; /* "Communication version" BCD */

    return iobuf_write(resp_buf, resp, sizeof(resp));
}

static HRESULT io3_cmd_get_features(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t req;
    HRESULT hr;

    hr = iobuf_read_8(req_buf, &req);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Get features\n");

    hr = iobuf_write_8(resp_buf, 0x01); /* Write report byte */

    if (FAILED(hr)) {
        return hr;
    }

    return iobuf_write(resp_buf, io3_features, sizeof(io3_features));
}

static HRESULT io3_cmd_read_switches(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_switches req;
    struct io3_switch_state state;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

#if 0
    dprintf("JVS I/O: Read switches, np=%i, bpp=%i\n",
            req.num_players,
            req.bytes_per_player);
#endif

    if (req.num_players > 2 || req.bytes_per_player != 2) {
        dprintf("JVS I/O: Invalid read size "
                        "num_players=%i "
                        "bytes_per_player=%i\n",
                req.num_players,
                req.bytes_per_player);
        hr = iobuf_write_8(resp_buf, 0x02);

        if (FAILED(hr)) {
            return hr;
        }

        return E_FAIL;
    }

    /* Build response */

    hr = iobuf_write_8(resp_buf, 0x01); /* Report byte */

    if (FAILED(hr)) {
        return hr;
    }

    memset(&state, 0, sizeof(state));

    if (io3->ops != NULL) {
        io3->ops->read_switches(io3->ops_ctx, &state);
    }

    hr = iobuf_write_8(resp_buf, state.system); /* Test, Tilt lines */

    if (FAILED(hr)) {
        return hr;
    }

    if (req.num_players > 0) {
        hr = iobuf_write_be16(resp_buf, state.p1);

        if (FAILED(hr)) {
            return hr;
        }
    }

    if (req.num_players > 1) {
        hr = iobuf_write_be16(resp_buf, state.p2);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;
}

static HRESULT io3_cmd_read_coin(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_coin req;
    uint16_t ncoins;
    uint8_t i;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    //dprintf("JVS I/O: Read coin, nslots=%i\n", req.nslots);

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write slot detail */

    for (i = 0 ; i < req.nslots ; i++) {
        if (io3->ops->read_coin_counter != NULL) {
            ncoins = io3->ops->read_coin_counter(io3->ops_ctx, i);
        } else {
            ncoins = 0;
        }

        hr = iobuf_write_be16(resp_buf, ncoins);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;
}

static HRESULT io3_cmd_read_analogs(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_read_analogs req;
    uint16_t state;
    uint8_t i;
    HRESULT hr;

    /* Read req */

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    //dprintf("JVS I/O: Read analogs, nanalogs=%i\n", req.nanalogs);

    /* Write report byte */

    hr = iobuf_write_8(resp_buf, 0x01);

    if (FAILED(hr)) {
        return hr;
    }

    /* Write analogs */

    for (i = 0 ; i < req.nanalogs ; i++) {
        if (io3->ops->read_analog != NULL) {
            state = io3->ops->read_analog(io3->ops_ctx, i);
        } else {
            state = 0;
        }

        hr = iobuf_write_be16(resp_buf, state);

        if (FAILED(hr)) {
            return hr;
        }
    }

    return hr;

}

static HRESULT io3_cmd_write_gpio(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    uint8_t cmd;
    uint8_t nbytes;
    uint8_t bytes[3];
    HRESULT hr;

    /* Read request header */

    hr = iobuf_read_8(req_buf, &cmd);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_read_8(req_buf, &nbytes);

    if (FAILED(hr)) {
        return hr;
    }

    if (nbytes > 3) {
        dprintf("JVS I/O: Invalid GPIO write size %i\n", nbytes);
        hr = iobuf_write_8(resp_buf, 0x02);

        if (FAILED(hr)) {
            return hr;
        }

        return E_FAIL;
    }

    /* Read payload */

    memset(bytes, 0, sizeof(bytes));
    hr = iobuf_read(req_buf, bytes, nbytes);

    if (FAILED(hr)) {
        return hr;
    }

    if (io3->ops->write_gpio != NULL) {
        io3->ops->write_gpio(
                io3->ops_ctx,
                bytes[0] | (bytes[1] << 8) | (bytes[2] << 16));
    }

    /* Write report byte */

    return iobuf_write_8(resp_buf, 0x01);
}

static HRESULT io3_cmd_reset(struct io3 *io3, struct const_iobuf *req_buf)
{
    struct jvs_req_reset req;
    HRESULT hr;

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS I/O: Reset (param %02x)\n", req.unknown);
    io3->addr = 0xFF;

    if (io3->ops->reset != NULL) {
        io3->ops->reset(io3->ops_ctx);
    }

    /* No ack for this since it really is addressed to everybody */

    return S_OK;
}

static HRESULT io3_cmd_assign_addr(
        struct io3 *io3,
        struct const_iobuf *req_buf,
        struct iobuf *resp_buf)
{
    struct jvs_req_assign_addr req;
    bool sense;
    HRESULT hr;

    hr = iobuf_read(req_buf, &req, sizeof(req));

    if (FAILED(hr)) {
        return hr;
    }

    sense = jvs_node_sense(io3->jvs.next);
    dprintf("JVS I/O: Assign addr %02x sense %i\n", req.addr, sense);

    if (sense) {
        /* That address is for somebody else */
        return S_OK;
    }

    io3->addr = req.addr;

    return iobuf_write_8(resp_buf, 0x01);
}
