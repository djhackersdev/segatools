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

struct nusec_log_record {
    uint8_t unknown[60];
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
static struct nusec_log_record nusec_log[7154];
static size_t nusec_log_head;
static size_t nusec_log_tail;

void nusec_hook_init(void)
{
    nusec_nearfull = 0x00010200;
    nusec_play_count = 0;
    nusec_play_limit = 1024;
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
    uint32_t count;
    size_t avail;
    HRESULT hr;

    hr = iobuf_read_le32(&irp->write, &count);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("Security: %s(count=%i)\n", __func__, count);

    avail = nusec_log_head - nusec_log_tail;

    if (count < avail) {
        count = avail;
    }

    nusec_log_tail += count;

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
    size_t used;
    size_t avail;

    used = nusec_log_head - nusec_log_tail;
    avail = _countof(nusec_log) - used;

    dprintf("Security: %s: used=%i avail=%i\n", __func__,
            (int) used,
            (int) avail);

    return iobuf_write_le32(&irp->read, avail);
}

static HRESULT nusec_ioctl_get_nvram_geometry(struct irp *irp)
{
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

         iobuf_write_le32(&irp->read, 10);      /* Num NVRAMs */
    hr = iobuf_write_le32(&irp->read, 4096);    /* Num addresses (per NVRAM?) */

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
    uint32_t pos;
    uint32_t count;
    size_t avail;
    HRESULT hr;

    dprintf("Security: %s\n", __func__);

    hr = iobuf_read_le32(&irp->write, &pos);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iobuf_read_le32(&irp->write, &count);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("    Params: %i %i Buf: %i\n", pos, count, irp->read.nbytes);

    avail = irp->read.nbytes - irp->read.pos;

    if (avail < count * sizeof(struct nusec_log_record)) {
        dprintf("\tError: Insufficient buffer\n");

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    while (count > 0 && pos != nusec_log_head) {
        memcpy( &irp->read.bytes[irp->read.pos],
                &nusec_log[pos % _countof(nusec_log)],
                sizeof(struct nusec_log_record));

        irp->read.pos += sizeof(struct nusec_log_record);
        pos++;
        count--;
    }

    return S_OK;
}

static HRESULT nusec_ioctl_get_trace_log_state(struct irp *irp)
{
    HRESULT hr;

    dprintf("Security: %s H: %i T: %i\n",
            __func__,
            (int) nusec_log_head,
            (int) nusec_log_tail);

         iobuf_write_le32(&irp->read, nusec_log_head - nusec_log_tail);
    hr = iobuf_write_le32(&irp->read, nusec_log_tail);

    return hr;
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

    if (irp->write.nbytes != sizeof(struct nusec_log_record)) {
        dprintf("    Log record size is incorrect\n");

        return E_INVALIDARG;
    }

    if (nusec_log_head - nusec_log_tail >= _countof(nusec_log)) {
        dprintf("    Log buffer is full!\n");

        return HRESULT_FROM_WIN32(ERROR_DISK_FULL);
    }

    memcpy( &nusec_log[nusec_log_head % _countof(nusec_log)],
            irp->write.bytes,
            sizeof(struct nusec_log_record));

    nusec_log_head++;

    dprintf("    H: %i T: %i\n", (int) nusec_log_head, (int) nusec_log_tail);

    return S_OK;
}
