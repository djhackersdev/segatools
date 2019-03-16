#include <windows.h>

#include <assert.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "board/slider-cmd.h"
#include "board/slider-frame.h"

#include "divahook/slider-hook.h"

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT slider_handle_irp(struct irp *irp);
static HRESULT slider_handle_irp_locked(struct irp *irp);

static HRESULT slider_req_dispatch(const union slider_req_any *req);
static HRESULT slider_req_reset(void);
static HRESULT slider_req_get_board_info(void);
static HRESULT slider_req_auto_scan_start(void);
static HRESULT slider_req_auto_scan_stop(void);
static HRESULT slider_req_set_led(const struct slider_req_set_led *req);

static unsigned int __stdcall slider_thread_proc(void *ctx);

static CRITICAL_SECTION slider_lock;
static struct uart slider_uart;
static uint8_t slider_written_bytes[520];
static uint8_t slider_readable_bytes[520];

static HANDLE slider_thread;
static bool slider_stop;

void slider_hook_init(void)
{
    InitializeCriticalSection(&slider_lock);

    uart_init(&slider_uart, 11);
    slider_uart.written.bytes = slider_written_bytes;
    slider_uart.written.nbytes = sizeof(slider_written_bytes);
    slider_uart.readable.bytes = slider_readable_bytes;
    slider_uart.readable.nbytes = sizeof(slider_readable_bytes);

    iohook_push_handler(slider_handle_irp);
}

static HRESULT slider_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&slider_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    EnterCriticalSection(&slider_lock);
    hr = slider_handle_irp_locked(irp);
    LeaveCriticalSection(&slider_lock);

    return hr;
}

static HRESULT slider_handle_irp_locked(struct irp *irp)
{
    union slider_req_any req;
    struct iobuf req_iobuf;
    HRESULT hr;

    hr = uart_handle_irp(&slider_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    for (;;) {
#if 0
        dprintf("TX Buffer:\n");
        dump_iobuf(&slider_uart.written);
#endif

        req_iobuf.bytes = req.bytes;
        req_iobuf.nbytes = sizeof(req.bytes);
        req_iobuf.pos = 0;

        hr = slider_frame_decode(&req_iobuf, &slider_uart.written);

        if (hr != S_OK) {
            if (FAILED(hr)) {
                dprintf("Diva slider: Deframe error: %x\n", (int) hr);
            }

            return hr;
        }

#if 0
        dprintf("Deframe Buffer:\n");
        dump_iobuf(&req_iobuf);
#endif

        hr = slider_req_dispatch(&req);

        if (FAILED(hr)) {
            dprintf("Diva slider: Processing error: %x\n", (int) hr);
        }
    }
}

static HRESULT slider_req_dispatch(const union slider_req_any *req)
{
    switch (req->hdr.cmd) {
    case SLIDER_CMD_RESET:
        return slider_req_reset();

    case SLIDER_CMD_GET_BOARD_INFO:
        return slider_req_get_board_info();

    case SLIDER_CMD_AUTO_SCAN_START:
        return slider_req_auto_scan_start();

    case SLIDER_CMD_AUTO_SCAN_STOP:
        return slider_req_auto_scan_stop();

    case SLIDER_CMD_SET_LED_DIVA:
        return slider_req_set_led(&req->set_led);

    default:
        dprintf("Unhandled command %02x\n", req->hdr.cmd);

        return S_OK;
    }
}

static HRESULT slider_req_reset(void)
{
    struct slider_hdr resp;

    dprintf("Diva slider: Reset\n");

    resp.sync = 0xFF;
    resp.cmd = SLIDER_CMD_RESET;
    resp.nbytes = 0;

    return slider_frame_encode(&slider_uart.readable, &resp, sizeof(resp));
}

static HRESULT slider_req_get_board_info(void)
{
    struct slider_resp_get_board_info resp;

    dprintf("Diva slider: Get firmware version\n");

    memset(&resp, 0, sizeof(resp));
    resp.hdr.sync = SLIDER_FRAME_SYNC;
    resp.hdr.cmd = SLIDER_CMD_GET_BOARD_INFO;
    resp.hdr.nbytes = sizeof(resp.version);

    memset(resp.version, 0, sizeof(resp.version));
    strcpy_s(
            resp.version,
            sizeof(resp.version),
            "15275   \xA0" "06712\xFF" "\x90");

    return slider_frame_encode(&slider_uart.readable, &resp, sizeof(resp));
}

static HRESULT slider_req_auto_scan_start(void)
{
    dprintf("Diva slider: Start slider thread\n");

    if (slider_thread != NULL) {
        dprintf("Thread is already running\n");

        return S_OK;
    }

    slider_thread = (HANDLE) _beginthreadex(
            NULL,
            0,
            slider_thread_proc,
            NULL,
            0,
            NULL);

    if (slider_thread == NULL) {
        dprintf("Thread launch failed\n");
    }

    /* This message is not acknowledged */

    return S_OK;
}

static HRESULT slider_req_auto_scan_stop(void)
{
    struct slider_hdr resp;

    dprintf("Diva slider: Stop slider thread\n");

    if (slider_thread != NULL) {
        slider_stop = true;
        LeaveCriticalSection(&slider_lock);

        WaitForSingleObject(slider_thread, INFINITE);
        CloseHandle(slider_thread);
        slider_thread = NULL;
        slider_stop = false;

        dprintf("Diva slider: Thread has terminated\n");

        EnterCriticalSection(&slider_lock);
    }

    resp.sync = SLIDER_FRAME_SYNC;
    resp.cmd = SLIDER_CMD_AUTO_SCAN_STOP;
    resp.nbytes = 0;

    return slider_frame_encode(&slider_uart.readable, &resp, sizeof(resp));
}

static HRESULT slider_req_set_led(const struct slider_req_set_led *req)
{
    /* This message is not acknowledged */
    return S_OK;
}

static unsigned int WINAPI slider_thread_proc(void *ctx)
{
    static const int keys[] = {
        'Q', 'W', 'E', 'R', 'U', 'I', 'O', 'P',
    };

    struct slider_resp_auto_scan resp;
    uint8_t pressure;
    bool stop;
    size_t i;

    for (;;) {
        resp.hdr.sync = SLIDER_FRAME_SYNC;
        resp.hdr.cmd = SLIDER_CMD_AUTO_SCAN;
        resp.hdr.nbytes = sizeof(resp.pressure);

        for (i = 0 ; i < 8 ; i++) {
            pressure = GetAsyncKeyState(keys[i]) ? 20 : 0;
            memset(&resp.pressure[28 - 4 * i], pressure, 4);
        }

        EnterCriticalSection(&slider_lock);

        stop = slider_stop;

        if (!stop) {
            slider_frame_encode(&slider_uart.readable, &resp, sizeof(resp));
        }

        LeaveCriticalSection(&slider_lock);

        if (stop) {
            return 0;
        }

        Sleep(1);
    }
}
