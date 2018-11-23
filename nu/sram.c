#include <windows.h>

#include <assert.h>

#include "hook/iohook.h"

#include "nu/sram.h"
#include "nu/nvram.h"

#include "util/dprintf.h"
#include "util/setupapi.h"

static HRESULT sram_handle_irp(struct irp *irp);
static HRESULT sram_handle_open(struct irp *irp);
static HRESULT sram_handle_close(struct irp *irp);
static HRESULT sram_handle_ioctl(struct irp *irp);

static HRESULT sram_ioctl_get_geometry(struct irp *irp);

static HANDLE sram_file;

HRESULT sram_hook_init(void)
{
    HRESULT hr;

    hr = nvram_open_file(&sram_file, L"DEVICE\\sram.bin", 0x80000);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iohook_push_handler(sram_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    hr = setupapi_add_phantom_dev(&sram_guid, L"$sram");

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT sram_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != sram_file) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return sram_handle_open(irp);
    case IRP_OP_CLOSE:  return sram_handle_close(irp);
    case IRP_OP_IOCTL:  return sram_handle_ioctl(irp);
    default:            return iohook_invoke_next(irp);
    }
}

static HRESULT sram_handle_open(struct irp *irp)
{
    if (wcscmp(irp->open_filename, L"$sram") != 0) {
        return iohook_invoke_next(irp);
    }

    dprintf("SRAM: Open device\n");
    irp->fd = sram_file;

    return S_OK;
}

static HRESULT sram_handle_close(struct irp *irp)
{
    dprintf("SRAM: Close device\n");

    return S_OK;
}

static HRESULT sram_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        return sram_ioctl_get_geometry(irp);

    default:
        dprintf("SRAM: Unknown ioctl %x, write %i read %i\n",
                irp->ioctl,
                irp->write.nbytes,
                irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT sram_ioctl_get_geometry(struct irp *irp)
{
    DISK_GEOMETRY *out;

    if (irp->read.nbytes < sizeof(*out)) {
        dprintf("SRAM: Invalid ioctl response buffer size\n");

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dprintf("SRAM: Get geometry\n");

    out = (DISK_GEOMETRY *) irp->read.bytes;
    out->Cylinders.QuadPart = 0x20000;
    out->MediaType = 0;
    out->TracksPerCylinder = 1;
    out->SectorsPerTrack = 1;
    out->BytesPerSector = 4;

    irp->read.pos = sizeof(*out);

    return S_OK;
}
