#ifndef NDEBUG

#include <assert.h>
#include <stddef.h>

#include "hook/iobuf.h"

#include "util/dprintf.h"
#include "util/dump.h"

void dump(const void *ptr, size_t nbytes)
{
    const uint8_t *bytes;
    size_t i;

    assert(ptr != NULL);

    if (nbytes == 0) {
        dprintf("\t--- Empty ---\n");
    }

    bytes = ptr;

    for (i = 0 ; i < nbytes ; i++) {
        if (i % 16 == 0) {
            dprintf("\t%08x:", i);
        }

        dprintf(" %02x", bytes[i]);

        if (i % 16 == 15) {
            dprintf("\n");
        }
    }

    dprintf("\n");
}

void dump_iobuf(const struct iobuf *iobuf)
{
    assert(iobuf != NULL);
    assert(iobuf->bytes != NULL || iobuf->nbytes == 0);
    assert(iobuf->pos <= iobuf->nbytes);

    dump(iobuf->bytes, iobuf->pos);
}

void dump_const_iobuf(const struct const_iobuf *iobuf)
{
    assert(iobuf != NULL);
    assert(iobuf->bytes != NULL || iobuf->nbytes == 0);
    assert(iobuf->pos <= iobuf->nbytes);

    dump(&iobuf->bytes[iobuf->pos], iobuf->nbytes - iobuf->pos);
}

#endif
