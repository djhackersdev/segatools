#include <stddef.h>
#include <stdint.h>

#include "util/crc.h"

uint32_t crc32(const void *src, size_t nbytes, uint32_t in)
{
    const uint8_t *bytes;
    uint32_t crc;
    size_t i;

    bytes = src;
    crc = ~in;

    for (i = 0 ; i < nbytes * 8 ; i++) {
        if (i % 8 == 0) {
            crc ^= *bytes++;
        }

        if (crc & 1) {
            crc = (crc >> 1) ^ 0xEDB88320;
        } else {
            crc = (crc >> 1);
        }
    }

    return ~crc;
}
