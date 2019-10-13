/* This is some sort of LCD display found on various cabinets. It is driven
   directly by amdaemon, and it has something to do with displaying the status
   of electronic payments.

   Part number in schematics is "VFD GP1232A02A FUTABA".

   Little else about this board is known. Black-holing the RS232 comms that it
   receives seems to be sufficient for the time being. */

#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "board/vfd.h"

#include "hook/iohook.h"

#include "hooklib/uart.h"

#include "util/dprintf.h"
#include "util/dump.h"

static HRESULT vfd_handle_irp(struct irp *irp);

static struct uart vfd_uart;
static uint8_t vfd_written[512];
static uint8_t vfd_readable[512];

HRESULT vfd_hook_init(unsigned int port_no)
{
    uart_init(&vfd_uart, port_no);
    vfd_uart.written.bytes = vfd_written;
    vfd_uart.written.nbytes = sizeof(vfd_written);
    vfd_uart.readable.bytes = vfd_readable;
    vfd_uart.readable.nbytes = sizeof(vfd_readable);

    return iohook_push_handler(vfd_handle_irp);
}

static HRESULT vfd_handle_irp(struct irp *irp)
{
    HRESULT hr;

    assert(irp != NULL);

    if (!uart_match_irp(&vfd_uart, irp)) {
        return iohook_invoke_next(irp);
    }

    hr = uart_handle_irp(&vfd_uart, irp);

    if (FAILED(hr) || irp->op != IRP_OP_WRITE) {
        return hr;
    }

    dprintf("VFD TX:\n");
    dump_iobuf(&vfd_uart.written);
    vfd_uart.written.pos = 0;

    return hr;
}
