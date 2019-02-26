#include <windows.h>

#include <assert.h>

#include "board/sg-cmd.h"
#include "board/sg-led.h"
#include "board/sg-led-cmd.h"

#include "util/dprintf.h"

static HRESULT sg_led_dispatch(
        void *ctx,
        const void *v_req,
        void *v_resp);

static HRESULT sg_led_cmd_reset(
        const struct sg_led *led,
        const struct sg_req_header *req,
        struct sg_led_resp_reset *resp);

static HRESULT sg_led_cmd_get_info(
        const struct sg_led *led,
        const struct sg_req_header *req,
        struct sg_led_resp_get_info *resp);

static HRESULT sg_led_cmd_set_color(
        const struct sg_led *led,
        const struct sg_led_req_set_color *req);

static const uint8_t sg_led_info[] = {
    '1', '5', '0', '8', '4', 0xFF, 0x10, 0x00, 0x12,
};

void sg_led_init(
        struct sg_led *led,
        uint8_t addr,
        const struct sg_led_ops *ops,
        void *ctx)
{
    assert(led != NULL);
    assert(ops != NULL);

    led->ops = ops;
    led->ops_ctx = ctx;
    led->addr = addr;
}

void sg_led_transact(
        struct sg_led *led,
        struct iobuf *resp_frame,
        const void *req_bytes,
        size_t req_nbytes)
{
    assert(led != NULL);
    assert(resp_frame != NULL);
    assert(req_bytes != NULL);

    sg_req_transact(resp_frame, req_bytes, req_nbytes, sg_led_dispatch, led);
}

#ifdef NDEBUG
#define sg_led_dprintfv(led, fmt, ap)
#define sg_led_dprintf(led, fmt, ...)
#else
static void sg_led_dprintfv(
        const struct sg_led *led,
        const char *fmt,
        va_list ap)
{
    dprintf("RGB LED %02x: ", led->addr);
    dprintfv(fmt, ap);
}

static void sg_led_dprintf(
        const struct sg_led *led,
        const char *fmt,
        ...)
{
    va_list ap;

    va_start(ap, fmt);
    sg_led_dprintfv(led, fmt, ap);
    va_end(ap);
}
#endif

static HRESULT sg_led_dispatch(
        void *ctx,
        const void *v_req,
        void *v_resp)
{
    const struct sg_led *led;
    const union sg_led_req_any *req;
    union sg_led_resp_any *resp;

    led = ctx;
    req = v_req;
    resp = v_resp;

    if (req->simple.hdr.addr != led->addr) {
        /* Not addressed to us, don't send a response */
        return S_FALSE;
    }

    switch (req->simple.hdr.cmd) {
    case SG_RGB_CMD_RESET:
        return sg_led_cmd_reset(led, &req->simple, &resp->reset);

    case SG_RGB_CMD_GET_INFO:
        return sg_led_cmd_get_info(led, &req->simple, &resp->get_info);

    case SG_RGB_CMD_SET_COLOR:
        return sg_led_cmd_set_color(led, &req->set_color);

    default:
        sg_led_dprintf(led, "Unimpl command %02x\n", req->simple.hdr.cmd);

        return E_NOTIMPL;
    }
}

static HRESULT sg_led_cmd_reset(
        const struct sg_led *led,
        const struct sg_req_header *req,
        struct sg_led_resp_reset *resp)
{
    HRESULT hr;

    sg_led_dprintf(led, "Reset\n");
    sg_resp_init(&resp->resp, req, sizeof(resp->payload));
    resp->payload = 0;

    if (led->ops->reset != NULL) {
        hr = led->ops->reset(led->ops_ctx);
    } else {
        hr = S_OK;
    }

    if (FAILED(hr)) {
        sg_led_dprintf(led, "led->ops->reset: Error %x\n", hr);
        return hr;
    }

    return S_OK;
}

static HRESULT sg_led_cmd_get_info(
        const struct sg_led *led,
        const struct sg_req_header *req,
        struct sg_led_resp_get_info *resp)
{
    sg_led_dprintf(led, "Get info\n");
    sg_resp_init(&resp->resp, req, sizeof(resp->payload));
    memcpy(resp->payload, sg_led_info, sizeof(sg_led_info));

    return S_OK;
}

static HRESULT sg_led_cmd_set_color(
        const struct sg_led *led,
        const struct sg_led_req_set_color *req)
{
    if (req->req.payload_len != sizeof(req->payload)) {
        sg_led_dprintf(led, "%s: Payload size is incorrect\n", __func__);

        goto fail;
    }

    led->ops->set_color(
            led->ops_ctx,
            req->payload[0],
            req->payload[1],
            req->payload[2]);

fail:
    /* No response */
    return S_FALSE;
}
