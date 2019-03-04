#include <windows.h>

#include <assert.h>
#include <string.h>

#include "util/dprintf.h"

#include "hook/iohook.h"

#include "platform/hwmon.h"

enum {
    HWMON_IOCTL_READ_CPU_TEMP = 0x80006000,
};

static HRESULT hwmon_handle_irp(struct irp *irp);
static HRESULT hwmon_handle_open(struct irp *irp);
static HRESULT hwmon_handle_close(struct irp *irp);
static HRESULT hwmon_handle_ioctl(struct irp *irp);

static HRESULT hwmon_ioctl_read_cpu_temp(struct irp *irp);

static HANDLE hwmon_fd;

void hwmon_hook_init(void)
{
    hwmon_fd = iohook_open_dummy_fd();
    iohook_push_handler(hwmon_handle_irp);
}

static HRESULT hwmon_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != hwmon_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return hwmon_handle_open(irp);
    case IRP_OP_CLOSE:  return hwmon_handle_close(irp);
    case IRP_OP_IOCTL:  return hwmon_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT hwmon_handle_open(struct irp *irp)
{
    if (wcscmp(irp->open_filename, L"\\\\.\\sghwmonitor") != 0) {
        return iohook_invoke_next(irp);
    }

    dprintf("Hwmon: Opened device\n");
    irp->fd = hwmon_fd;

    return S_OK;
}

static HRESULT hwmon_handle_close(struct irp *irp)
{
    dprintf("Hwmon: Closed device\n");

    return S_OK;
}

static HRESULT hwmon_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case HWMON_IOCTL_READ_CPU_TEMP:
        return hwmon_ioctl_read_cpu_temp(irp);

    default:
        dprintf("Hwmon: Unknown ioctl %08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT hwmon_ioctl_read_cpu_temp(struct irp *irp)
{
    /* Assuming this is Celsius. It also seems to be biased by -23 (based on the
       caller code) despite being a 32-bit int. */

    dprintf("Hwmon: Read CPU temperature\n");

    return iobuf_write_le32(&irp->read, 52);
}
