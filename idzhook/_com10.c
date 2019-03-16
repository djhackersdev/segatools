#include <windows.h>

#include <assert.h>
#include <stdint.h>

#include "board/sg-reader.h"

#include "hook/iohook.h"

#include "idzhook/_com10.h"

static HRESULT com10_handle_irp(struct irp *irp);

static struct sg_reader com10_reader;

HRESULT com10_hook_init(void)
{
    HRESULT hr;

    hr = sg_reader_init(&com10_reader, 10);

    if (FAILED(hr)) {
        return hr;
    }

    return iohook_push_handler(com10_handle_irp);
}

static HRESULT com10_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (!sg_reader_match_irp(&com10_reader, irp)) {
        return iohook_invoke_next(irp);
    }

    return sg_reader_handle_irp(&com10_reader, irp);
}
