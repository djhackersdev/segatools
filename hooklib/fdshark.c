#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "hook/iobuf.h"
#include "hook/iohook.h"

#include "hooklib/fdshark.h"

#include "util/dprintf.h"
#include "util/dump.h"

static const wchar_t *fdshark_path;
static HANDLE fdshark_target_fd;
static int fdshark_flags;

static HRESULT fdshark_handle_irp(struct irp *irp);
static HRESULT fdshark_handle_open(struct irp *irp);
static HRESULT fdshark_handle_close(struct irp *irp);
static HRESULT fdshark_handle_read(struct irp *irp);
static HRESULT fdshark_handle_write(struct irp *irp);
static HRESULT fdshark_handle_ioctl(struct irp *irp);
static bool fdshark_force_sync(struct irp *irp, HRESULT hr);

HRESULT fdshark_hook_init(const wchar_t *path, int flags)
{
    assert(path != NULL);
    assert(!(flags & ~FDSHARK_ALL_FLAGS_));

    fdshark_path = path;
    fdshark_flags = flags;

    return iohook_push_handler(fdshark_handle_irp);
}

static HRESULT fdshark_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != fdshark_target_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return fdshark_handle_open(irp);
    case IRP_OP_CLOSE:  return fdshark_handle_close(irp);
    case IRP_OP_READ:   return fdshark_handle_read(irp);
    case IRP_OP_WRITE:  return fdshark_handle_write(irp);
    case IRP_OP_IOCTL:  return fdshark_handle_ioctl(irp);
    default:            return iohook_invoke_next(irp);
    }
}

static HRESULT fdshark_handle_open(struct irp *irp)
{
    HRESULT hr;

    if (_wcsicmp(irp->open_filename, fdshark_path) != 0) {
        return iohook_invoke_next(irp);
    }

    hr = iohook_invoke_next(irp);

    if (FAILED(hr)) {
        return hr;
    }

    dprintf("FdShark: Opened %S\n", fdshark_path);
    fdshark_target_fd = irp->fd;

    return hr;
}

static HRESULT fdshark_handle_close(struct irp *irp)
{
    dprintf("FdShark: Closed %S\n", fdshark_path);
    fdshark_target_fd = NULL;

    return iohook_invoke_next(irp);
}

static HRESULT fdshark_handle_read(struct irp *irp)
{
    HRESULT hr;

    if (!(fdshark_flags & FDSHARK_TRACE_READ)) {
        return iohook_invoke_next(irp);
    }

    dprintf("FdShark: Read %p:%i/%i\n",
            irp->read.bytes,
            (int) irp->read.pos,
            (int) irp->read.nbytes);

    hr = iohook_invoke_next(irp);

    if (FAILED(hr) && !fdshark_force_sync(irp, hr)) {
        dprintf("FdShark: FAILED: %x\n", (int) hr);
    } else {
        dprintf("FdShark: Read %p:%i/%i OK\n",
                irp->read.bytes,
                (int) irp->read.pos,
                (int) irp->read.nbytes);
        dump_iobuf(&irp->read);
    }

    return S_OK;
}

static HRESULT fdshark_handle_write(struct irp *irp)
{
    HRESULT hr;

    if (!(fdshark_flags & FDSHARK_TRACE_WRITE)) {
        return iohook_invoke_next(irp);
    }

    dprintf("FdShark: Write %p:%i/%i\n",
            irp->write.bytes,
            (int) irp->write.pos,
            (int) irp->write.nbytes);
    dump_const_iobuf(&irp->write);

    hr = iohook_invoke_next(irp);

    if (FAILED(hr) && !fdshark_force_sync(irp, hr)) {
        dprintf("FdShark: FAILED: %x\n", (int) hr);
    } else {
        dprintf("FdShark: Write %p:%i/%i OK\n",
                irp->write.bytes,
                (int) irp->write.pos,
                (int) irp->write.nbytes);
    }

    return S_OK;
}

static HRESULT fdshark_handle_ioctl(struct irp *irp)
{
    HRESULT hr;

    if (!(fdshark_flags & FDSHARK_TRACE_IOCTL)) {
        return iohook_invoke_next(irp);
    }

    dprintf("FdShark: Ioctl %08x w:%p:%i/%i r:%p:%i/%i\n",
            irp->ioctl,
            irp->write.bytes,
            (int) irp->write.pos,
            (int) irp->write.nbytes,
            irp->read.bytes,
            (int) irp->read.pos,
            (int) irp->read.nbytes);
    dump_const_iobuf(&irp->write);

    hr = iohook_invoke_next(irp);

    if (FAILED(hr) && !fdshark_force_sync(irp, hr)) {
        dprintf("FdShark: FAILED: %x\n", (int) hr);
    } else {
        dprintf("FdShark: Ioctl %08x w:%p:%i/%i r:%p:%i/%i OK\n",
                irp->ioctl,
                irp->write.bytes,
                (int) irp->write.pos,
                (int) irp->write.nbytes,
                irp->read.bytes,
                (int) irp->read.pos,
                (int) irp->read.nbytes);
        dump_iobuf(&irp->read);
    }

    return S_OK;
}

static bool fdshark_force_sync(struct irp *irp, HRESULT hr)
{
    DWORD xferred;
    BOOL ok;

    if (!(fdshark_flags & FDSHARK_FORCE_SYNC)) {
        return false;
    }

    if (    hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING) &&
            hr != HRESULT_FROM_NT(STATUS_PENDING)) {
        return false;
    }

    ok = GetOverlappedResult(irp->fd, irp->ovl, &xferred, TRUE);

    if (!ok) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("FdShark: Synchronous block failed: %x\n", (int) hr);

        return false;
    }

    switch (irp->op) {
    case IRP_OP_READ:
    case IRP_OP_IOCTL:
        irp->read.pos += xferred;

        break;

    case IRP_OP_WRITE:
        irp->write.pos += xferred;

        break;

    default:
        break;
    }

    return true;
}
