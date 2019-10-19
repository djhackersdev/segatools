#pragma once

#include <stdint.h>

struct mifare_block {
    uint8_t bytes[16];
};

struct mifare_sector {
    struct mifare_block blocks[4];
};

struct mifare {
    struct mifare_sector sectors[16];
};
