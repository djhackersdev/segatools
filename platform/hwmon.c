#include <windows.h>

#include <assert.h>
#include <string.h>

#include "hook/iohook.h"

#include "platform/hwmon.h"

#include "util/dprintf.h"
#include "util/str.h"

enum {
    HWMON_IOCTL_READ_CPU_TEMP = 0x80006000,
};

static HRESULT hwmon_handle_irp(struct irp *irp);
static HRESULT hwmon_handle_open(struct irp *irp);
static HRESULT hwmon_handle_close(struct irp *irp);
static HRESULT hwmon_handle_ioctl(struct irp *irp);

static HRESULT hwmon_ioctl_read_cpu_temp(struct irp *irp);

static HANDLE hwmon_fd;

HRESULT hwmon_hook_init(const struct hwmon_config *cfg)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    hr = iohook_open_nul_fd(&hwmon_fd);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iohook_push_handler(hwmon_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
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
    if (!wstr_ieq(irp->open_filename, L"\\\\.\\sghwmonitor")) {
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

    return iobuf_write_le32(&irp->read, 52);
}
