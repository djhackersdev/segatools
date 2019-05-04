#include <stdbool.h>
#include <stdint.h>

#include "idzio/shifter.h"

static bool idz_shifter_shifting;
static uint8_t idz_shifter_gear;

void idz_shifter_reset(void)
{
    idz_shifter_gear = 0;
}

void idz_shifter_update(bool shift_dn, bool shift_up)
{
    if (!idz_shifter_shifting) {
        if (shift_dn && idz_shifter_gear > 0) {
            idz_shifter_gear--;
        }

        if (shift_up && idz_shifter_gear < 6) {
            idz_shifter_gear++;
        }
    }

    idz_shifter_shifting = shift_dn || shift_up;
}

uint8_t idz_shifter_current_gear(void)
{
    return idz_shifter_gear;
}
