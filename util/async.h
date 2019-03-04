#pragma once

#include <windows.h>

#include <stdbool.h>

#include "hook/iohook.h"

typedef HRESULT (*async_task_t)(void *ctx, struct irp *irp);

struct async {
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE pend;
    CONDITION_VARIABLE avail;
    HANDLE thread;
    struct irp irp;
    async_task_t task;
    void *ctx;
    bool stop;
};

void async_init(struct async *async, void *ctx);
void async_fini(struct async *async);
HRESULT async_submit(struct async *async, struct irp *irp, async_task_t task);
