#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "idzio/backend.h"
#include "idzio/idzio.h"
#include "idzio/xi.h"

static const struct idz_io_backend *idz_io_backend;
static bool idz_io_coin;
static uint16_t idz_io_coins;

HRESULT idz_io_init(void)
{
    assert(idz_io_backend == NULL);

    return idz_xi_init(&idz_io_backend);
}

void idz_io_jvs_read_buttons(uint8_t *opbtn_out, uint8_t *gamebtn_out)
{
    uint8_t opbtn;

    assert(idz_io_backend != NULL);
    assert(opbtn_out != NULL);
    assert(gamebtn_out != NULL);

    opbtn = 0;

    if (GetAsyncKeyState('1')) {
        opbtn |= IDZ_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState('2')) {
        opbtn |= IDZ_IO_OPBTN_SERVICE;
    }

    *opbtn_out = opbtn;

    idz_io_backend->jvs_read_buttons(gamebtn_out);
}

void idz_io_jvs_read_shifter(uint8_t *gear)
{
    assert(gear != NULL);
    assert(idz_io_backend != NULL);

    idz_io_backend->jvs_read_shifter(gear);
}

void idz_io_jvs_read_analogs(struct idz_io_analog_state *state)
{
    assert(state != NULL);
    assert(idz_io_backend != NULL);

    idz_io_backend->jvs_read_analogs(state);
}

void idz_io_jvs_read_coin_counter(uint16_t *out)
{
    assert(out != NULL);

    /* Coin counter is not backend-specific */

    if (GetAsyncKeyState('3')) {
        if (!idz_io_coin) {
            idz_io_coin = true;
            idz_io_coins++;
        }
    } else {
        idz_io_coin = false;
    }

    *out = idz_io_coins;
}
