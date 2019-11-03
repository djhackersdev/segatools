#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hook/iohook.h"

#include "hooklib/reg.h"

#include "platform/config.h"
#include "platform/nusec.h"

#include "util/dprintf.h"
#include "util/dump.h"
#include "util/str.h"

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

static HRESULT nusec_reg_read_game_id(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_keychip_id(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_model_type(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_platform_id(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_region(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_server_ip_ipv4(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_server_ip_ipv6(void *bytes, uint32_t *nbytes);
static HRESULT nusec_reg_read_system_flag(void *bytes, uint32_t *nbytes);

static const struct reg_hook_val nusec_reg_vals[] = {
    {
        .name   = L"gameId",
        .read   = nusec_reg_read_game_id,
        .type   = REG_BINARY,
    }, {
        .name   = L"keychipId",
        .read   = nusec_reg_read_keychip_id,
        .type   = REG_BINARY,
    }, {
        .name   = L"modelType",
        .read   = nusec_reg_read_model_type,
        .type   = REG_DWORD,
    }, {
        .name   = L"platformId",
        .read   = nusec_reg_read_platform_id,
        .type   = REG_BINARY,
    }, {
        .name   = L"region",
        .read   = nusec_reg_read_region,
        .type   = REG_DWORD,
    }, {
        .name   = L"serverIpIpv4",
        .read   = nusec_reg_read_server_ip_ipv4,
        .type   = REG_BINARY,
    }, {
        .name   = L"serverIpIpv6",
        .read   = nusec_reg_read_server_ip_ipv6,
        .type   = REG_BINARY,
    }, {
        .name   = L"systemFlag",
        .read   = nusec_reg_read_system_flag,
        .type   = REG_DWORD,
    }
};

static HANDLE nusec_fd;
static uint32_t nusec_nearfull;
static uint32_t nusec_play_count;
static uint32_t nusec_play_limit;
static struct nusec_log_record nusec_log[7154];
static size_t nusec_log_head;
static size_t nusec_log_tail;
static struct nusec_config nusec_cfg;

HRESULT nusec_hook_init(
        const struct nusec_config *cfg,
        const char *game_id,
        const char *platform_id)
{
    HRESULT hr;

    assert(cfg != NULL);
    assert(game_id != NULL && strlen(game_id) == sizeof(cfg->game_id));
    assert(platform_id != NULL && strlen(platform_id) == sizeof(cfg->platform_id));

    if (!cfg->enable) {
        return S_FALSE;
    }

    memcpy(&nusec_cfg, cfg, sizeof(*cfg));

    if (nusec_cfg.game_id[0] == '\0') {
        memcpy(nusec_cfg.game_id, game_id, sizeof(nusec_cfg.game_id));
    }

    if (nusec_cfg.platform_id[0] == '\0') {
        memcpy(nusec_cfg.platform_id, platform_id, sizeof(nusec_cfg.platform_id));
    }

    nusec_nearfull = 0x00010200;
    nusec_play_count = 0;
    nusec_play_limit = 1024;

    hr = iohook_open_nul_fd(&nusec_fd);

    if (FAILED(hr)) {
        return hr;
    }

    hr = iohook_push_handler(nusec_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    hr = reg_hook_push_key(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\SEGA\\SystemProperty\\keychip",
            nusec_reg_vals,
            _countof(nusec_reg_vals));

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
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
    if (!wstr_ieq(irp->open_filename, L"\\??\\FddDriver")) {
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
        dprintf("Security: Unknown ioctl %#08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT nusec_ioctl_ping(struct irp *irp)
{
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
            nusec_cfg.billing_ca,
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

    dprintf(">>> %p:%i/%i\n",
            irp->read.bytes,
            (int) irp->read.pos,
            (int) irp->read.nbytes);

    hr = iohook_invoke_next(irp);

    dprintf("<<< %p:%i/%i\n",
            irp->read.bytes,
            (int) irp->read.pos,
            (int) irp->read.nbytes);

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
            nusec_cfg.billing_pub,
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

    dprintf(">>> %p:%i/%i\n",
            irp->read.bytes,
            (int) irp->read.pos,
            (int) irp->read.nbytes);

    hr = iohook_invoke_next(irp);

    dprintf("<<< %p:%i/%i\n",
            irp->read.bytes,
            (int) irp->read.pos,
            (int) irp->read.nbytes);

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

    dprintf("    Params: %i %i Buf: %i\n", pos, count, (int) irp->read.nbytes);

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

static HRESULT nusec_reg_read_game_id(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_bin(
            bytes,
            nbytes,
            &nusec_cfg.game_id,
            sizeof(nusec_cfg.game_id));
}

static HRESULT nusec_reg_read_keychip_id(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_bin(
            bytes,
            nbytes,
            &nusec_cfg.keychip_id,
            sizeof(nusec_cfg.keychip_id));
}

static HRESULT nusec_reg_read_model_type(void *bytes, uint32_t *nbytes)
{
    uint32_t u32;
    char c;

    c = nusec_cfg.platform_id[3];

    if (c >= '0' && c <= '9') {
        u32 = c - '0';
    } else {
        u32 = 0;
    }

    return reg_hook_read_u32(bytes, nbytes, u32);
}

static HRESULT nusec_reg_read_platform_id(void *bytes, uint32_t *nbytes)
{
    /* Fourth byte is a separate registry val (modelType).
       We store it in the same config field for ease of configuration. */

    return reg_hook_read_bin(
            bytes,
            nbytes,
            &nusec_cfg.platform_id,
            3);
}

static HRESULT nusec_reg_read_region(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, nusec_cfg.region);
}

static HRESULT nusec_reg_read_server_ip_ipv4(void *bytes, uint32_t *nbytes)
{
    uint32_t subnet;

    subnet = _byteswap_ulong(nusec_cfg.subnet);

    return reg_hook_read_bin(bytes, nbytes, &subnet, sizeof(subnet));
}

static HRESULT nusec_reg_read_server_ip_ipv6(void *bytes, uint32_t *nbytes)
{
    uint8_t subnet[16];

    memset(subnet, 0, sizeof(subnet));

    return reg_hook_read_bin(bytes, nbytes, subnet, sizeof(subnet));
}

static HRESULT nusec_reg_read_system_flag(void *bytes, uint32_t *nbytes)
{
    return reg_hook_read_u32(bytes, nbytes, nusec_cfg.system_flag);
}
