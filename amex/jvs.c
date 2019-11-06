#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>

#include <ntstatus.h>

#include <assert.h>
#include <stddef.h>

#include "amex/jvs.h"

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/setupapi.h"

#include "jvs/jvs-bus.h"

#include "util/dprintf.h"
#include "util/dump.h"
#include "util/str.h"

enum {
    JVS_IOCTL_HELLO     = 0x80006004,
    JVS_IOCTL_SENSE     = 0x8000600C,
    JVS_IOCTL_TRANSACT  = 0x8000E008,
};

static HRESULT jvs_handle_irp(struct irp *irp);
static HRESULT jvs_handle_open(struct irp *irp);
static HRESULT jvs_handle_close(struct irp *irp);
static HRESULT jvs_handle_ioctl(struct irp *irp);

static HRESULT jvs_ioctl_hello(struct irp *irp);
static HRESULT jvs_ioctl_sense(struct irp *irp);
static HRESULT jvs_ioctl_transact(struct irp *irp);

static HANDLE jvs_fd;
static struct jvs_node *jvs_root;
static jvs_provider_t jvs_provider;

HRESULT jvs_hook_init(const struct jvs_config *cfg, jvs_provider_t provider)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    hr = iohook_push_handler(jvs_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    hr = setupapi_add_phantom_dev(&jvs_guid, L"$jvs");

    if (FAILED(hr)) {
        return hr;
    }

    jvs_provider = provider;

    return S_OK;
}

static HRESULT jvs_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != jvs_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return jvs_handle_open(irp);
    case IRP_OP_CLOSE:  return jvs_handle_close(irp);
    case IRP_OP_IOCTL:  return jvs_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT jvs_handle_open(struct irp *irp)
{
    struct jvs_node *root;
    HRESULT hr;

    if (!wstr_eq(irp->open_filename, L"$jvs")) {
        return iohook_invoke_next(irp);
    }

    if (jvs_fd != NULL) {
        dprintf("JVS Port: Already open\n");

        return HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
    }

    hr = iohook_open_nul_fd(&jvs_fd);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("JVS Port: Open device\n");

    if (jvs_provider != NULL) {
        hr = jvs_provider(&root);

        if (SUCCEEDED(hr)) {
            jvs_root = root;
        }
    }

    irp->fd = jvs_fd;

    return S_OK;
}

static HRESULT jvs_handle_close(struct irp *irp)
{
    dprintf("JVS Port: Close device\n");
    jvs_fd = NULL;

    return iohook_invoke_next(irp);
}

static HRESULT jvs_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case JVS_IOCTL_HELLO:
        return jvs_ioctl_hello(irp);

    case JVS_IOCTL_SENSE:
        return jvs_ioctl_sense(irp);

    case JVS_IOCTL_TRANSACT:
        return jvs_ioctl_transact(irp);

    default:
        dprintf("JVS Port: Unknown ioctl %#x\n", irp->ioctl);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT jvs_ioctl_hello(struct irp *irp)
{
    HRESULT hr;

    // uuh fucked if i know

    dprintf("JVS Port: Port startup (?)\n");

         iobuf_write_8(&irp->read, 0);
    hr = iobuf_write_8(&irp->read, 0);

    return hr;
}

static HRESULT jvs_ioctl_sense(struct irp *irp)
{
    uint8_t code;
    bool sense;

    if (jvs_root != NULL) {
        sense = jvs_root->sense(jvs_root);

        if (sense) {
            dprintf("JVS Port: Sense line 2.5 V (address unassigned)\n");
            code = 3;
        } else {
            dprintf("JVS Port: Sense line 0.0 V (address assigned)\n");
            code = 2;
        }
    } else {
        dprintf("JVS Port: Sense line 5.0 V (no downstream PCB)\n");
        code = 1;
    }

    return iobuf_write_8(&irp->read, code);
}

static HRESULT jvs_ioctl_transact(struct irp *irp)
{
#if 0
    dprintf("\nJVS Port: Outbound frame:\n");
    dump_const_iobuf(&irp->write);
#endif

    jvs_bus_transact(jvs_root, irp->write.bytes, irp->write.nbytes, &irp->read);

#if 0
    dprintf("JVS Port: Inbound frame:\n");
    dump_iobuf(&irp->read);
    dprintf("\n");
#endif

    if (irp->read.pos == 0) {
        /* The un-acked JVS reset command must return ERROR_NO_DATA_DETECTED,
           and this error must always be returned asynchronously. And since
           async I/O comes from the NT kernel, we have to return that win32
           error as the equivalent NTSTATUS. */

        if (irp->ovl == NULL || irp->ovl->hEvent == NULL) {
            return HRESULT_FROM_WIN32(ERROR_NO_DATA_DETECTED);
        }

        irp->ovl->Internal = STATUS_NO_DATA_DETECTED;
        SetEvent(irp->ovl->hEvent);

        return HRESULT_FROM_WIN32(ERROR_IO_PENDING);
    } else {
        return S_OK;
    }
}
