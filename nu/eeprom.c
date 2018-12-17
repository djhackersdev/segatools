#include <windows.h>

#ifdef __GNUC__
#include <ntdef.h>
#else
#include <winnt.h>
#endif
#include <devioctl.h>
#include <ntdddisk.h>

#include <assert.h>

#include "hook/iohook.h"

#include "nu/eeprom.h"
#include "nu/nvram.h"

#include "util/dprintf.h"
#include "util/setupapi.h"

static HRESULT eeprom_handle_irp(struct irp *irp);
static HRESULT eeprom_handle_open(struct irp *irp);
static HRESULT eeprom_handle_close(struct irp *irp);
static HRESULT eeprom_handle_ioctl(struct irp *irp);
static HRESULT eeprom_handle_read(struct irp *irp);
static HRESULT eeprom_handle_write(struct irp *irp);

static HRESULT eeprom_ioctl_get_geometry(struct irp *irp);

static HANDLE eeprom_file;

HRESULT eeprom_hook_init(void)
{
    HRESULT hr;

    hr = nvram_open_file(&eeprom_file, L"DEVICE\\eeprom.bin", 0x2000);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iohook_push_handler(eeprom_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    hr = setupapi_add_phantom_dev(&eeprom_guid, L"$eeprom");

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT eeprom_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != eeprom_file) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return eeprom_handle_open(irp);
    case IRP_OP_CLOSE:  return eeprom_handle_close(irp);
    case IRP_OP_IOCTL:  return eeprom_handle_ioctl(irp);
    case IRP_OP_READ:   return eeprom_handle_read(irp);
    case IRP_OP_WRITE:  return eeprom_handle_write(irp);
    default:            return iohook_invoke_next(irp);
    }
}

static HRESULT eeprom_handle_open(struct irp *irp)
{
    if (wcscmp(irp->open_filename, L"$eeprom") != 0) {
        return iohook_invoke_next(irp);
    }

    dprintf("EEPROM: Open device\n");
    irp->fd = eeprom_file;

    return S_OK;
}

static HRESULT eeprom_handle_close(struct irp *irp)
{
    dprintf("EEPROM: Close device\n");

    return S_OK;
}

static HRESULT eeprom_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        return eeprom_ioctl_get_geometry(irp);

    default:
        dprintf("EEPROM: Unknown ioctl %x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT eeprom_ioctl_get_geometry(struct irp *irp)
{
    DISK_GEOMETRY *out;

    if (irp->read.nbytes < sizeof(*out)) {
        dprintf("EEPROM: Invalid ioctl response buffer size\n");

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dprintf("EEPROM: Get geometry\n");

    /* Not the real values, just bullshitting something for now */

    out = (DISK_GEOMETRY *) irp->read.bytes;
    out->Cylinders.QuadPart = 0x800;
    out->MediaType = 0;
    out->TracksPerCylinder = 1;
    out->SectorsPerTrack = 2;
    out->BytesPerSector = 4;

    irp->read.pos = sizeof(*out);

    return S_OK;
}

static HRESULT eeprom_handle_read(struct irp *irp)
{
    if (irp->ovl == NULL) {
        dprintf("EEPROM: Synchronous read..?\n");

        return E_UNEXPECTED;
    }

    dprintf("EEPROM: Read off %x len %x\n",
            (int) irp->ovl->Offset,
            (int) irp->read.nbytes);

    return iohook_invoke_next(irp);
}

static HRESULT eeprom_handle_write(struct irp *irp)
{
    if (irp->ovl == NULL) {
        dprintf("EEPROM: Synchronous write..?\n");

        return E_UNEXPECTED;
    }

    dprintf("EEPROM: Write off %x len %x\n",
            (int) irp->ovl->Offset,
            (int) irp->write.nbytes);

    return iohook_invoke_next(irp);
}
