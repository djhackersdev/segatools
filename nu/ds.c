#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>

#ifdef __GNUC__
#include <ntdef.h>
#else
#include <winnt.h>
#endif
#include <devioctl.h>
#include <ntdddisk.h>
#include <ntstatus.h>

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "nu/ds.h"
#include "nu/nvram.h"

#include "util/crc.h"
#include "util/dprintf.h"
#include "util/setupapi.h"

#pragma pack(push, 1)

enum {
    DS_IOCTL_SETUP          = 0x80006004,
    DS_IOCTL_READ_SECTOR    = 0x80006010,
};

struct ds_eeprom {
    uint32_t crc32;
    uint8_t unk_04[5];
    char serial_no[17];
    uint8_t unk_1A[6];
};

static_assert(sizeof(struct ds_eeprom) == 0x20, "DS EEPROM size");

#pragma pack(pop)

static HRESULT ds_handle_irp(struct irp *irp);
static HRESULT ds_handle_open(struct irp *irp);
static HRESULT ds_handle_close(struct irp *irp);
static HRESULT ds_handle_ioctl(struct irp *irp);

static HRESULT ds_ioctl_get_geometry(struct irp *irp);
static HRESULT ds_ioctl_setup(struct irp *irp);
static HRESULT ds_ioctl_read_sector(struct irp *irp);

static const char ds_serial_file[] = "DEVICE/ds.txt";
static struct ds_eeprom ds_eeprom;
static HANDLE ds_fd;

HRESULT ds_hook_init(void)
{
    HRESULT hr;
    FILE *f;
    char c;
    int i;

    memset(&ds_eeprom, 0, sizeof(ds_eeprom));

    f = fopen(ds_serial_file, "r");

    if (f != NULL) {
        i = 0;

        for (;;) {
            if (feof(f) || i >= sizeof(ds_eeprom.serial_no) - 1) {
                break;
            }

            c = getc(f);

            if (isspace(c)) {
                break;
            }

            ds_eeprom.serial_no[i++] = c;
        }

        fclose(f);
    } else {
        dprintf("Failed to open %s\n", ds_serial_file);
    }

    ds_eeprom.crc32 = crc32(&ds_eeprom.unk_04, 0x1C, 0);

    hr = iohook_push_handler(ds_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    hr = setupapi_add_phantom_dev(&ds_guid, L"$ds");

    if (FAILED(hr)) {
        return hr;
    }

    ds_fd = iohook_open_dummy_fd();

    return S_OK;
}

static HRESULT ds_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != ds_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return ds_handle_open(irp);
    case IRP_OP_CLOSE:  return ds_handle_close(irp);
    case IRP_OP_IOCTL:  return ds_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT ds_handle_open(struct irp *irp)
{
    if (wcscmp(irp->open_filename, L"$ds") != 0) {
        return iohook_invoke_next(irp);
    }

    dprintf("DS: Open device\n");
    irp->fd = ds_fd;

    return S_OK;
}

static HRESULT ds_handle_close(struct irp *irp)
{
    dprintf("DS: Close device\n");

    return S_OK;
}

static HRESULT ds_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        return ds_ioctl_get_geometry(irp);

    case DS_IOCTL_SETUP:
        return ds_ioctl_setup(irp);

    case DS_IOCTL_READ_SECTOR:
        return ds_ioctl_read_sector(irp);

    default:
        dprintf("DS: Unknown ioctl %08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT ds_ioctl_get_geometry(struct irp *irp)
{
    DISK_GEOMETRY *out;

    if (irp->read.nbytes < sizeof(*out)) {
        dprintf("DS: Invalid ioctl response buffer size\n");

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    dprintf("DS: Get geometry\n");

    out = (DISK_GEOMETRY *) irp->read.bytes;
    out->Cylinders.QuadPart = 1;
    out->MediaType = 0;
    out->TracksPerCylinder = 1;
    out->SectorsPerTrack = 2;
    out->BytesPerSector = 32;

    irp->read.pos = sizeof(*out);

    return S_OK;
}

static HRESULT ds_ioctl_setup(struct irp *irp)
{
    dprintf("DS: Setup IOCTL\n");

    return S_OK;
}

static HRESULT ds_ioctl_read_sector(struct irp *irp)
{
    struct const_iobuf src;
    uint32_t sector_no;
    HRESULT hr;

    hr = iobuf_read_le32(&irp->write, &sector_no);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("DS: Read sector %08x\n", sector_no);

    src.bytes = (const uint8_t *) &ds_eeprom;
    src.nbytes = sizeof(ds_eeprom);
    src.pos = 0;

    iobuf_move(&irp->read, &src);

    if (irp->ovl != NULL) {
        irp->ovl->Internal = STATUS_SUCCESS;
        irp->ovl->InternalHigh = irp->read.pos;

        if (irp->ovl->hEvent != NULL) {
            SetEvent(irp->ovl->hEvent);
        }
    }

    return S_OK;
}
