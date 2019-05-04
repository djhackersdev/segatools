#pragma once

#include <stdbool.h>
#include <stdint.h>

void idz_shifter_reset(void);
void idz_shifter_update(bool shift_dn, bool shift_up);
uint8_t idz_shifter_current_gear(void);
