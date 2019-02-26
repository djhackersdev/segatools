#pragma once

#include <windows.h>

#include <stddef.h>
#include <stdint.h>

HRESULT aime_io_init(void);
void aime_io_fini(void);
HRESULT aime_io_mifare_poll(uint8_t unit_no, uint32_t *uid);
HRESULT aime_io_mifare_read_luid(
        uint8_t unit_no,
        uint32_t uid,
        uint8_t *luid,
        size_t luid_size);
void aime_io_led_set_color(uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b);
