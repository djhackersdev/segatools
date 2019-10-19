#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "iccard/aime.h"
#include "iccard/mifare.h"

#include "util/dprintf.h"

HRESULT aime_card_populate(
        struct mifare *mifare,
        const uint8_t *luid,
        size_t nbytes)
{
    uint8_t b;
    size_t i;

    assert(mifare != NULL);
    assert(luid != NULL);

    memset(mifare, 0, sizeof(*mifare));

    if (nbytes != 10) {
        dprintf("AiMe IC: LUID must be 10 bytes\n");

        return E_INVALIDARG;
    }

    for (i = 0 ; i < 10 ; i++) {
        b = luid[i];

        if ((b & 0xF0) > 0x90 || (b & 0x0F) > 0x09) {
            dprintf("AiMe IC: LUID must be binary-coded decimal\n");
            return E_INVALIDARG;
        }

        mifare->sectors[0].blocks[2].bytes[6 + i] = b;
    }

    /* TODO An authentic Aime pass has a checksum of the LUID in the last few
       bytes of block 1. The output of this function fails authenticity check
       in its current form. */

    dprintf("AiMe IC: WARNING: Authenticity hash not yet implemented!\n");

    return S_OK;
}
