#pragma once

#include <stdint.h>

#include "idzio/idzio.h"

struct idz_io_backend {
    void (*jvs_read_buttons)(uint8_t *gamebtn);
    void (*jvs_read_shifter)(uint8_t *gear);
    void (*jvs_read_analogs)(struct idz_io_analog_state *state);
};
