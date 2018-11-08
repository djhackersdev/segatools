#include <windows.h>
#include <assert.h>
#include <string.h>

#include "util/dprintf.h"
#include "util/dump.h"

#include "hook/iohook.h"

enum {
    NUSEC_IOCTL_PING                    = 0x22A114,
    NUSEC_IOCTL_ERASE_TRACE_LOG         = 0x22E188,
    NUSEC_IOCTL_ADD_PLAY_COUNT          = 0x22E154,
    NUSEC_IOCTL_GET_BILLING_CA_CERT     = 0x22E1C4,
    NUSEC_IOCTL_GET_BILLING_PUBKEY      = 0x22E1C8,
    NUSEC_IOCTL_GET_NEARFULL            = 0x22E20C,
    NUSEC_IOCTL_GET_NVRAM_AVAILABLE     = 0x22E19C,
    NUSEC_IOCTL_GET_NVRAM_GEOMETRY      = 0x22E24C,
    NUSEC_IOCTL_GET_PLAY_COUNT          = 0x22E150,
    NUSEC_IOCTL_GET_PLAY_LIMIT          = 0x22E204,
    NUSEC_IOCTL_GET_TRACE_LOG_DATA      = 0x22E194,
    NUSEC_IOCTL_GET_TRACE_LOG_STATE     = 0x22E198,
    NUSEC_IOCTL_PUT_NEARFULL            = 0x22E210,
    NUSEC_IOCTL_PUT_PLAY_LIMIT          = 0x22E208,
    NUSEC_IOCTL_PUT_TRACE_LOG_DATA      = 0x22E190,
};

static HRESULT nusec_handle_irp(struct irp *irp);
static HRESULT nusec_handle_open(struct irp *irp);
static HRESULT nusec_handle_close(struct irp *irp);
static HRESULT nusec_handle_ioctl(struct irp *irp);

static HRESULT nusec_ioctl_ping(struct irp *irp);
static HRESULT nusec_ioctl_erase_trace_log(struct irp *irp);
static HRESULT nusec_ioctl_add_play_count(struct irp *irp);
static HRESULT nusec_ioctl_get_billing_ca_cert(struct irp *irp);
static HRESULT nusec_ioctl_get_billing_pubkey(struct irp *irp);
static HRESULT nusec_ioctl_get_nearfull(struct irp *irp);
static HRESULT nusec_ioctl_get_nvram_available(struct irp *irp);
static HRESULT nusec_ioctl_get_nvram_geometry(struct irp *irp);
static HRESULT nusec_ioctl_get_play_count(struct irp *irp);
static HRESULT nusec_ioctl_get_play_limit(struct irp *irp);
static HRESULT nusec_ioctl_get_trace_log_data(struct irp *irp);
static HRESULT nusec_ioctl_get_trace_log_state(struct irp *irp);
static HRESULT nusec_ioctl_put_nearfull(struct irp *irp);
static HRESULT nusec_ioctl_put_play_limit(struct irp *irp);
static HRESULT nusec_ioctl_put_trace_log_data(struct irp *irp);

static HANDLE nusec_fd;
static uint32_t nusec_nearfull;
static uint32_t nusec_play_count;
static uint32_t nusec_play_limit;

void nusec_hook_init(void)
{
    nusec_nearfull = 0x00010200;
    nusec_play_count = 0;
    nusec_play_limit = 1000;
    nusec_fd = iohook_open_dummy_fd();
    iohook_push_handler(nusec_handle_irp);
}

static HRESULT nusec_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != nusec_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return nusec_handle_open(irp);
    case IRP_OP_CLOSE:  return nusec_handle_close(irp);
    case IRP_OP_IOCTL:  return nusec_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT nusec_handle_open(struct irp *irp)
{
    if (wcscmp(irp->open_filename, L"\\??\\FddDriver") != 0) {
        return iohook_invoke_next(irp);
    }

    dprintf("Security: Opened handle\n");
    irp->fd = nusec_fd;

    return S_OK;
}

static HRESULT nusec_handle_close(struct irp *irp)
{
    dprintf("Security: Closed handle\n");

    return S_OK;
}

static HRESULT nusec_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case NUSEC_IOCTL_PING:
        return nusec_ioctl_ping(irp);

    case NUSEC_IOCTL_ERASE_TRACE_LOG:
        return nusec_ioctl_erase_trace_log(irp);

    case NUSEC_IOCTL_ADD_PLAY_COUNT:
        return nusec_ioctl_add_play_count(irp);

    case NUSEC_IOCTL_GET_BILLING_CA_CERT:
        return nusec_ioctl_get_billing_ca_cert(irp);

    case NUSEC_IOCTL_GET_BILLING_PUBKEY:
        return nusec_ioctl_get_billing_pubkey(irp);

    case NUSEC_IOCTL_GET_NEARFULL:
        return nusec_ioctl_get_nearfull(irp);

    case NUSEC_IOCTL_GET_NVRAM_AVAILABLE:
        return nusec_ioctl_get_nvram_available(irp);

    case NUSEC_IOCTL_GET_NVRAM_GEOMETRY:
        return nusec_ioctl_get_nvram_geometry(irp);

    case NUSEC_IOCTL_GET_PLAY_COUNT:
        return nusec_ioctl_get_play_count(irp);

    case NUSEC_IOCTL_GET_PLAY_LIMIT:
        return nusec_ioctl_get_play_limit(irp);

    case NUSEC_IOCTL_GET_TRACE_LOG_DATA:
        return nusec_ioctl_get_trace_log_data(irp);

    case NUSEC_IOCTL_GET_TRACE_LOG_STATE:
        return nusec_ioctl_get_trace_log_state(irp);

    case NUSEC_IOCTL_PUT_NEARFULL:
        return nusec_ioctl_put_nearfull(irp);

    case NUSEC_IOCTL_PUT_PLAY_LIMIT:
        return nusec_ioctl_put_play_limit(irp);

    case NUSEC_IOCTL_PUT_TRACE_LOG_DATA:
        return nusec_ioctl_put_trace_log_data(irp);

    default:
        dprintf("Security: Unhandled ioctl %#08x: read %i write %i\n",
                irp->ioctl,
                irp->read.nbytes,
                irp->write.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT nusec_ioctl_ping(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);

    return S_OK;
}

static HRESULT nusec_ioctl_erase_trace_log(struct irp *irp)
{
    uint32_t cell;
    HRESULT hr;

    hr = iobuf_read_le32(&irp->write, &cell);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("Security: %s(cell=%i)\n", __func__, cell);

    return S_OK;
}

static HRESULT nusec_ioctl_add_play_count(struct irp *irp)
{
    uint32_t delta;
    HRESULT hr;

    hr = iobuf_read_le32(&irp->write, &delta);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("Security: Add play count: %i + %i = %i\n",
            nusec_play_count,
            delta,
            nusec_play_count + delta);

    nusec_play_count += delta;

    return iobuf_write_le32(&irp->read, nusec_play_count);
}

static HRESULT nusec_ioctl_get_billing_ca_cert(struct irp *irp)
{
    HANDLE fd;
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

    fd = CreateFileW(
            L"DEVICE\\ca.crt",
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (fd == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Error opening CA cert file: %x\n", (int) hr);

        return hr;
    }

    /* Transform this ioctl into a read from that file and pass the IRP on */

    irp->op = IRP_OP_READ;
    irp->fd = fd;

    dprintf(">>> %p:%i/%i\n", irp->read.bytes, irp->read.pos, irp->read.nbytes);
    hr = iohook_invoke_next(irp);
    dprintf("<<< %p:%i/%i\n", irp->read.bytes, irp->read.pos, irp->read.nbytes);

    if (FAILED(hr)) {
        dprintf("ReadFile transformation failed: %x\n", (int) hr);
    }

    CloseHandle(fd);

    return hr;
}

static HRESULT nusec_ioctl_get_billing_pubkey(struct irp *irp)
{
    HANDLE fd;
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

    fd = CreateFileW(
            L"DEVICE\\billing.pub",
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

    if (fd == INVALID_HANDLE_VALUE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("Error opening billing pubkey file: %x\n", (int) hr);

        return hr;
    }

    irp->op = IRP_OP_READ;
    irp->fd = fd;

    dprintf(">>> %p:%i/%i\n", irp->read.bytes, irp->read.pos, irp->read.nbytes);
    hr = iohook_invoke_next(irp);
    dprintf("<<< %p:%i/%i\n", irp->read.bytes, irp->read.pos, irp->read.nbytes);

    if (FAILED(hr)) {
        dprintf("ReadFile transformation failed: %x\n", (int) hr);
    }

    CloseHandle(fd);

    return hr;
}

static HRESULT nusec_ioctl_get_nearfull(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);

    return iobuf_write_le32(&irp->read, nusec_nearfull);
}

static HRESULT nusec_ioctl_get_nvram_available(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);

    return iobuf_write_le32(&irp->read, 1024);
}

static HRESULT nusec_ioctl_get_nvram_geometry(struct irp *irp)
{
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

         iobuf_write_le32(&irp->read, 10);
    hr = iobuf_write_le32(&irp->read, 4096);

    return hr;
}

static HRESULT nusec_ioctl_get_play_count(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);

    return iobuf_write_le32(&irp->read, nusec_play_count);
}

static HRESULT nusec_ioctl_get_play_limit(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);

    return iobuf_write_le32(&irp->read, nusec_play_limit);
}

static HRESULT nusec_ioctl_get_trace_log_data(struct irp *irp)
{
    uint32_t param0;
    uint32_t param1;
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

    hr = iobuf_read_le32(&irp->write, &param0);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_read_le32(&irp->write, &param1);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("\tPARAM %i %i\n", param0, param1);

    return E_FAIL;
}

static HRESULT nusec_ioctl_get_trace_log_state(struct irp *irp)
{
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

    hr = iobuf_write_le32(&irp->read, 0);

    if (FAILED(hr)) {
        return hr;
    }

    return iobuf_write_le32(&irp->read, 0);
}

static HRESULT nusec_ioctl_put_nearfull(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);
    dump_const_iobuf(&irp->write);

    return iobuf_read_le32(&irp->write, &nusec_nearfull);
}

static HRESULT nusec_ioctl_put_play_limit(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);
    dump_const_iobuf(&irp->write);

    return iobuf_read_le32(&irp->write, &nusec_play_limit);
}

static HRESULT nusec_ioctl_put_trace_log_data(struct irp *irp)
{
    dprintf("Security: %s\n", __func__);
    dump_const_iobuf(&irp->write);

    return S_OK;
}
