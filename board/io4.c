#include <windows.h>

#include <devioctl.h>
#include <hidclass.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "board/config.h"
#include "board/guid.h"
#include "board/io4.h"

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/setupapi.h"

#include "util/async.h"
#include "util/dprintf.h"

#pragma pack(push, 1)

enum {
    IO4_CMD_SET_COMM_TIMEOUT   = 0x01,
    IO4_CMD_SET_SAMPLING_COUNT = 0x02,
    IO4_CMD_CLEAR_BOARD_STATUS = 0x03,
    IO4_CMD_SET_GENERAL_OUTPUT = 0x04,
    IO4_CMD_SET_PWM_OUTPUT     = 0x05,
    IO4_CMD_UPDATE_FIRMWARE    = 0x85,
};

struct io4_report_in {
    uint8_t report_id;
    uint16_t adcs[8];
    uint16_t spinners[4];
    uint16_t chutes[2];
    uint16_t buttons[2];
    uint8_t system_status;
    uint8_t usb_status;
    uint8_t unknown[29];
};

static_assert(sizeof(struct io4_report_in) == 0x40, "IO4 IN report size");

struct io4_report_out {
    uint8_t report_id;
    uint8_t cmd;
    uint8_t payload[62];
};

static_assert(sizeof(struct io4_report_out) == 0x40, "IO4 OUT report size");

#pragma pack(pop)


static HRESULT io4_handle_irp(struct irp *irp);
static HRESULT io4_handle_open(struct irp *irp);
static HRESULT io4_handle_close(struct irp *irp);
static HRESULT io4_handle_read(struct irp *irp);
static HRESULT io4_handle_write(struct irp *irp);
static HRESULT io4_handle_ioctl(struct irp *irp);

static HRESULT io4_ioctl_get_manufacturer_string(struct irp *irp);
static HRESULT io4_ioctl_get_product_string(struct irp *irp);

static HRESULT io4_async_poll(void *ctx, struct irp *irp);

/* Device node path must contain substring "vid_0ca3" (case-insensitive). */
static const wchar_t io4_path[] = L"$io4\\vid_0ca3";

static const wchar_t io4_manf[] = L"SEGA";
static const wchar_t io4_prod[] =
        /* "Product" (N.B. numbers are in hex) */

        L"I/O CONTROL BD;"          /* Board type */
        L"15257;"                   /* Board number */
        L"01;"                      /* "Mode" (prob. USB vs JVS?) */
        L"90;"                      /* Firmware revision */
        L"1831;"                    /* Firmware checksum */
        L"6679A;"                   /* "Custom chip no" */
        L"00;"                      /* "Config" */

        /* "Function" (N.B. all values are in hex) */

        L"GOUT=14_"                 /* General-purpose output */
        L"ADIN=8,E_"                /* ADC inputs */
        L"ROTIN=4_"                 /* Rotary inputs */
        L"COININ=2_"                /* Coin inputs */
        L"SWIN=2,E_"                /* Switch inputs */
        L"UQ1=41,6"                 /* "Unique function 1" */
        ;

static HANDLE io4_fd;
static struct async io4_async;
static uint8_t io4_system_status;
static const struct io4_ops *io4_ops;
static void *io4_ops_ctx;

HRESULT io4_hook_init(
        const struct io4_config *cfg,
        const struct io4_ops *ops,
        void *ctx)
{
    HRESULT hr;

    assert(cfg != NULL);
    assert(ops != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    async_init(&io4_async, NULL);

    hr = iohook_open_nul_fd(&io4_fd);

    if (FAILED(hr)) {
        return hr;
    }

    io4_ops = ops;
    io4_ops_ctx = ctx;
    io4_system_status = 0x02; /* idk */
    iohook_push_handler(io4_handle_irp);

    hr = setupapi_add_phantom_dev(&hid_guid, io4_path);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT io4_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != io4_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return io4_handle_open(irp);
    case IRP_OP_CLOSE:  return io4_handle_close(irp);
    case IRP_OP_READ:   return io4_handle_read(irp);
    case IRP_OP_WRITE:  return io4_handle_write(irp);
    case IRP_OP_IOCTL:  return io4_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT io4_handle_open(struct irp *irp)
{
    if (wcscmp(irp->open_filename, io4_path) != 0) {
        return iohook_invoke_next(irp);
    }

    dprintf("USB I/O: Device opened\n");
    irp->fd = io4_fd;

    return S_OK;
}

static HRESULT io4_handle_close(struct irp *irp)
{
    dprintf("USB I/O: Device closed\n");

    return S_OK;
}

static HRESULT io4_handle_read(struct irp *irp)
{
    /* The amdaemon USBIO driver will continuously poll the IO until the IO
       call returns an async operation in progress. We have to return and then
       signal the OVERLAPPED event object "a little bit later" in order to avoid
       an infinite loop. */

    return async_submit(&io4_async, irp, io4_async_poll);
}

static HRESULT io4_handle_write(struct irp *irp)
{
    struct io4_report_out out;
    HRESULT hr;

    hr = iobuf_read(&irp->write, &out, sizeof(out));

    if (FAILED(hr)) {
        return hr;
    }

    if (out.report_id != 0x10) {
        dprintf("USB I/O: OUT Report ID is incorrect");

        return E_FAIL;
    }

    switch (out.cmd) {
    case IO4_CMD_SET_COMM_TIMEOUT:
        dprintf("USB I/O: Set comm timeout\n");

        // Ongeki Summer expects the system status to be 0x30 at this point
        io4_system_status = 0x30;

        return S_OK;

    case IO4_CMD_SET_SAMPLING_COUNT:
        dprintf("USB I/O: Set sampling count\n");

        // Ongeki Summer expects the system status to be 0x30 at this point
        io4_system_status = 0x30;

        return S_OK;

    case IO4_CMD_CLEAR_BOARD_STATUS:
        dprintf("USB I/O: Clear board status\n");
        io4_system_status = 0x00;

        return S_OK;

    case IO4_CMD_SET_GENERAL_OUTPUT:
        dprintf("USB I/O: GPIO Out\n");

        return S_OK;

    case IO4_CMD_SET_PWM_OUTPUT:
        dprintf("USB I/O: PWM Out\n");

        return S_OK;

    case IO4_CMD_UPDATE_FIRMWARE:
        dprintf("USB I/O: Update firmware..?\n");

        return E_FAIL;

    default:
        dprintf("USB I/O: Unknown command %02x\n", out.cmd);

        return E_FAIL;
    }
}

static HRESULT io4_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case IOCTL_HID_GET_MANUFACTURER_STRING:
        return io4_ioctl_get_manufacturer_string(irp);

    case IOCTL_HID_GET_PRODUCT_STRING:
        return io4_ioctl_get_product_string(irp);

    case IOCTL_HID_GET_INPUT_REPORT:
        dprintf("USB I/O: Control IN (untested!!)\n");

        return io4_handle_read(irp);

    case IOCTL_HID_SET_OUTPUT_REPORT:
        dprintf("USB I/O: Control OUT (untested!!)\n");

        return io4_handle_write(irp);

    default:
        dprintf("USB I/O: Unknown ioctl %#08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT io4_ioctl_get_manufacturer_string(struct irp *irp)
{
    dprintf("USB I/O: Get manufacturer string\n");

    if (irp->read.nbytes < sizeof(io4_manf)) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    memcpy(irp->read.bytes, io4_manf, sizeof(io4_manf));
    irp->read.pos = sizeof(io4_manf);

    return S_OK;
}

static HRESULT io4_ioctl_get_product_string(struct irp *irp)
{
    dprintf("USB I/O: Get product string\n");

    if (irp->read.nbytes < sizeof(io4_prod)) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    memcpy(irp->read.bytes, io4_prod, sizeof(io4_prod));
    irp->read.pos = sizeof(io4_prod);

    return S_OK;
}

static HRESULT io4_async_poll(void *ctx, struct irp *irp)
{
    struct io4_report_in in;
    struct io4_state state;
    HRESULT hr;
    size_t i;

    /* Delay long enough for the instigating thread in amdaemon to be satisfied
       that all queued-up reports have been drained. */

    Sleep(1);

    /* Call into ops to poll the underlying inputs */

    memset(&state, 0, sizeof(state));
    hr = io4_ops->poll(io4_ops_ctx, &state);

    if (FAILED(hr)) {
        return hr;
    }

    /* Construct IN report. Values are all little-endian, unlike JVS. */

    memset(&in, 0, sizeof(in));
    in.report_id = 0x01;
    in.system_status = io4_system_status;

    for (i = 0 ; i < 8 ; i++) {
        in.adcs[i] = state.adcs[i];
    }

    for (i = 0 ; i < 4 ; i++) {
        in.spinners[i] = state.spinners[i];
    }

    for (i = 0 ; i < 2 ; i++) {
        in.chutes[i] = state.chutes[i];
    }

    for (i = 0 ; i < 2 ; i++) {
        in.buttons[i] = state.buttons[i];
    }

    return iobuf_write(&irp->read, &in, sizeof(in));
}
