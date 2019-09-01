/* NTSTATUS chicanery. See precompiled.h */
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <winternl.h>
#include <ntstatus.h>

#include <assert.h>
#include <process.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hook/iohook.h"

#include "util/async.h"

static unsigned int __stdcall async_thread_proc(void *param);

void async_init(struct async *async, void *ctx)
{
    assert(async != NULL);

    InitializeCriticalSection(&async->lock);
    InitializeConditionVariable(&async->pend);
    InitializeConditionVariable(&async->avail);
    async->thread = NULL;
    async->ctx = ctx;
    async->stop = false;
}

void async_fini(struct async *async)
{
    HANDLE thread;

    if (async == NULL) {
        return;
    }

    EnterCriticalSection(&async->lock);

    async->stop = true;
    thread = async->thread;

    WakeConditionVariable(&async->pend);
    LeaveCriticalSection(&async->lock);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    DeleteCriticalSection(&async->lock);

    /* There is no DeleteConditionVariable function in the Win32 API. */
}

HRESULT async_submit(struct async *async, struct irp *irp, async_task_t task)
{
    BOOL ok;

    assert(async != NULL);
    assert(irp != NULL);
    assert(task != NULL);

    if (irp->ovl == NULL) {
        /* If there's no OVERLAPPED struct then just execute synchronously */
        return task(async->ctx, irp);
    }

    EnterCriticalSection(&async->lock);

    if (async->thread == NULL) {
        /* Ensure our worker thread is running */
        async->thread = (HANDLE) _beginthreadex(
                NULL,
                0,
                async_thread_proc,
                async,
                0,
                NULL);

        if (async->thread == NULL) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    while (async->task != NULL) {
        ok = SleepConditionVariableCS(&async->avail, &async->lock, INFINITE);

        if (!ok) {
            abort();
        }
    }

    async->task = task;
    memcpy(&async->irp, irp, sizeof(*irp));
    async->irp.next_handler = (size_t) -1;
    irp->ovl->Internal = STATUS_PENDING;

    WakeConditionVariable(&async->pend);
    LeaveCriticalSection(&async->lock);

    return HRESULT_FROM_WIN32(ERROR_IO_PENDING);
}

static unsigned int __stdcall async_thread_proc(void *param)
{
    struct async *async;
    struct irp irp;
    async_task_t task;
    OVERLAPPED *ovl;
    HANDLE event;
    HRESULT hr;
    BOOL ok;

    async = param;

    for (;;) {
        EnterCriticalSection(&async->lock);

        if (async->stop) {
            LeaveCriticalSection(&async->lock);

            break;
        } else if (async->task == NULL) {
            ok = SleepConditionVariableCS(&async->pend, &async->lock, INFINITE);

            if (!ok) {
                abort();
            }

            LeaveCriticalSection(&async->lock);
        } else {
            memcpy(&irp, &async->irp, sizeof(irp));
            task = async->task;
            ovl = async->irp.ovl;
            async->task = NULL;

            WakeConditionVariable(&async->avail);
            LeaveCriticalSection(&async->lock);

            assert(ovl != NULL);

            hr = task(async->ctx, &irp);

            switch (irp.op) {
            case IRP_OP_READ:
            case IRP_OP_IOCTL:
                ovl->InternalHigh = (DWORD) irp.read.pos;

                break;

            case IRP_OP_WRITE:
                ovl->InternalHigh = (DWORD) irp.write.pos;

                break;

            default:
                break;
            }

            /* We have to do a slightly tricky dance with the hooked process'
               call to GetOverlappedResult() here. This thread might be blocked
               on ovl->hEvent, or it might be just about to read ovl->Internal
               to determine whether the IO has completed (and thus determine
               whether it needs to block on ovl->hEvent or not). So to avoid
               any races and *ovl getting invalidated under our feet we must
               wake the initiating thread as follows:

               1. Take a local copy of ovl->hEvent

               2. Issue a memory fence to ensure that the previous load does
                  not get re-ordered after the following store

                  https://bartoszmilewski.com/2008/11/05/who-ordered-memory-fences-on-an-x86/

               3. Store the operation's NTSTATUS. At the moment that this store
                  gets issued the memory pointed to by ovl ceases to be safely
                  accessible.

               4. Using our local copy of the event handle (if present), signal
                  the initiating thread to wake up and retire the IO. */

            event = ovl->hEvent;
            MemoryBarrier();

            if (SUCCEEDED(hr)) {
                ovl->Internal = STATUS_SUCCESS;
            } else if (hr & FACILITY_NT_BIT) {
                ovl->Internal = hr & ~FACILITY_NT_BIT;
            } else {
                ovl->Internal = STATUS_UNSUCCESSFUL;
            }

            if (event != NULL) {
                SetEvent(event);
            }
        }
    }

    return 0;
}
