#include <windows.h>

#ifdef __GNUC__
#include <ntdef.h>
#else
#include <winnt.h>
#endif
#include <devioctl.h>
#include <ntdddisk.h>

#include <assert.h>

#include "amex/eeprom.h"
#include "amex/nvram.h"

#include "hook/iohook.h"

#include "hooklib/setupapi.h"

#include "util/dprintf.h"
#include "util/str.h"

enum {
    EEPROM_IOCTL_GET_ABI_VERSION = 0x80006000,
};

static HRESULT eeprom_handle_irp(struct irp *irp);
static HRESULT eeprom_handle_open(struct irp *irp);
static HRESULT eeprom_handle_close(struct irp *irp);
static HRESULT eeprom_handle_ioctl(struct irp *irp);
static HRESULT eeprom_handle_read(struct irp *irp);
static HRESULT eeprom_handle_write(struct irp *irp);

static HRESULT eeprom_ioctl_get_geometry(struct irp *irp);
static HRESULT eeprom_ioctl_get_abi_version(struct irp *irp);

static struct eeprom_config eeprom_config;
static HANDLE eeprom_file;

HRESULT eeprom_hook_init(const struct eeprom_config *cfg)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    memcpy(&eeprom_config, cfg, sizeof(*cfg));

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
    HRESULT hr;

    if (!wstr_eq(irp->open_filename, L"$eeprom") != 0) {
        return iohook_invoke_next(irp);
    }

    if (eeprom_file != NULL) {
        dprintf("EEPROM: Already open\n");

        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
    }

    dprintf("EEPROM: Open device\n");
    hr = nvram_open_file(&eeprom_file, eeprom_config.path, 0x2000);

    if (FAILED(hr)) {
        return hr;
    }

    irp->fd = eeprom_file;

    return S_OK;
}

static HRESULT eeprom_handle_close(struct irp *irp)
{
    dprintf("EEPROM: Close device\n");
    eeprom_file = NULL;

    return iohook_invoke_next(irp);
}

static HRESULT eeprom_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        return eeprom_ioctl_get_geometry(irp);

    case EEPROM_IOCTL_GET_ABI_VERSION:
        return eeprom_ioctl_get_abi_version(irp);

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
    DISK_GEOMETRY out;
    HRESULT hr;

    dprintf("EEPROM: Get geometry\n");

    memset(&out, 0, sizeof(out));
    out.Cylinders.QuadPart = 1;
    out.MediaType = FixedMedia;
    out.TracksPerCylinder = 224;
    out.SectorsPerTrack = 32;
    out.BytesPerSector = 1;

    hr = iobuf_write(&irp->read, &out, sizeof(out));

    if (FAILED(hr)) {
        dprintf("EEPROM: Get geometry failed: %08x\n", (int) hr);
    }

    return hr;
}

static HRESULT eeprom_ioctl_get_abi_version(struct irp *irp)
{
    return iobuf_write_le16(&irp->read, 256);
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
