#include <windows.h>

#include <assert.h>

#include "board/sg-reader.h"

#include "cardhook/_com12.h"

#include "hook/iohook.h"

static HRESULT com12_handle_irp(struct irp *irp);

static struct sg_reader com12_reader;

HRESULT com12_hook_init(void)
{
    HRESULT hr;

    hr = sg_reader_init(&com12_reader, 12);

    if (FAILED(hr)) {
        return hr;
    }

    return iohook_push_handler(com12_handle_irp);
}

static HRESULT com12_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (!sg_reader_match_irp(&com12_reader, irp)) {
        return iohook_invoke_next(irp);
    }

    return sg_reader_handle_irp(&com12_reader, irp);
}
