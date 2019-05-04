#pragma once

#include <windows.h>

#include <stdint.h>

enum {
    IDZ_IO_OPBTN_TEST = 0x01,
    IDZ_IO_OPBTN_SERVICE = 0x02,
};

enum {
    IDZ_IO_GAMEBTN_UP = 0x01,
    IDZ_IO_GAMEBTN_DOWN = 0x02,
    IDZ_IO_GAMEBTN_LEFT = 0x04,
    IDZ_IO_GAMEBTN_RIGHT = 0x08,
    IDZ_IO_GAMEBTN_START = 0x10,
    IDZ_IO_GAMEBTN_VIEW_CHANGE = 0x20,
};

struct idz_io_analog_state {
    int16_t wheel;
    uint16_t accel;
    uint16_t brake;
};

HRESULT idz_io_init(void);

void idz_io_jvs_read_analogs(struct idz_io_analog_state *out);

void idz_io_jvs_read_buttons(uint8_t *opbtn, uint8_t *gamebtn);

void idz_io_jvs_read_shifter(uint8_t *gear);

uint16_t idz_io_jvs_read_coin_counter(void);

// TODO force feedback once that gets reverse engineered
