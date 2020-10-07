#include <windows.h>

#include <assert.h>
#include <string.h>

#include "hook/iohook.h"

#include "platform/hwreset.h"

#include "util/dprintf.h"
#include "util/str.h"

enum {
    HWRESET_IOCTL_RESTART = 0x80002000,
};

static HRESULT hwreset_handle_irp(struct irp *irp);
static HRESULT hwreset_handle_open(struct irp *irp);
static HRESULT hwreset_handle_close(struct irp *irp);
static HRESULT hwreset_handle_ioctl(struct irp *irp);

static HRESULT hwreset_ioctl_restart(struct irp *irp);

static HANDLE hwreset_fd;

HRESULT hwreset_hook_init(const struct hwreset_config *cfg)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    hr = iohook_open_nul_fd(&hwreset_fd);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iohook_push_handler(hwreset_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT hwreset_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != hwreset_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return hwreset_handle_open(irp);
    case IRP_OP_CLOSE:  return hwreset_handle_close(irp);
    case IRP_OP_IOCTL:  return hwreset_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT hwreset_handle_open(struct irp *irp)
{
    if (!wstr_ieq(irp->open_filename, L"\\\\.\\sghwreset")) {
        return iohook_invoke_next(irp);
    }

    dprintf("Hwreset: Opened device\n");
    irp->fd = hwreset_fd;

    return S_OK;
}

static HRESULT hwreset_handle_close(struct irp *irp)
{
    dprintf("Hwreset: Closed device\n");

    return S_OK;
}

static HRESULT hwreset_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case HWRESET_IOCTL_RESTART:
        return hwreset_ioctl_restart(irp);

    default:
        dprintf("Hwreset: Unknown ioctl %08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT hwreset_ioctl_restart(struct irp *irp)
{
    dprintf("Hwreset: Reset requested\n");

    return iobuf_write_le32(&irp->read, 1);
}
