#ifndef NDEBUG

#include <assert.h>
#include <stddef.h>

#include "hook/iobuf.h"

#include "util/dprintf.h"
#include "util/dump.h"

void dump(const void *ptr, size_t nbytes)
{
    const uint8_t *bytes;
    uint8_t c;
    size_t i;
    size_t j;

    assert(ptr != NULL || nbytes == 0);

    if (nbytes == 0) {
        dprintf("\t--- Empty ---\n");
    }

    bytes = ptr;

    for (i = 0 ; i < nbytes ; i += 16) {
        dprintf("    %08x:", (int) i);

        for (j = 0 ; i + j < nbytes && j < 16 ; j++) {
            dprintf(" %02x", bytes[i + j]);
        }

        while (j < 16) {
            dprintf("   ");
            j++;
        }

        dprintf(" ");

        for (j = 0 ; i + j < nbytes && j < 16 ; j++) {
            c = bytes[i + j];

            if (c < 0x20 || c >= 0x7F) {
                c = '.';
            }

            dprintf("%c", c);
        }

        dprintf("\n");
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
