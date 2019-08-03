#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

#include "amex/jvs.h"

#include "board/io3.h"

#include "idzhook/jvs.h"

#include "idzio/idzio.h"

#include "jvs/jvs-bus.h"

#include "util/dprintf.h"

static void idz_jvs_read_analogs(
        void *ctx,
        uint16_t *analogs,
        uint8_t nanalogs);
static void idz_jvs_read_switches(void *ctx, struct io3_switch_state *out);
static void idz_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out);

static const struct io3_ops idz_jvs_io3_ops = {
    .read_switches      = idz_jvs_read_switches,
    .read_analogs       = idz_jvs_read_analogs,
    .read_coin_counter  = idz_jvs_read_coin_counter,
};

static const uint16_t idz_jvs_gear_signals[] = {
    /* Neutral */
    0x0000,
    /* 1: Left|Up */
    0x2800,
    /* 2: Left|Down */
    0x1800,
    /* 3: Up */
    0x2000,
    /* 4: Down */
    0x1000,
    /* 5: Right|Up */
    0x2400,
    /* 6: Right|Down */
    0x1400,
};

static struct io3 idz_jvs_io3;

HRESULT idz_jvs_init(void)
{
    HRESULT hr;

    hr = idz_io_init();

    if (FAILED(hr)) {
        return hr;
    }

    io3_init(&idz_jvs_io3, NULL, &idz_jvs_io3_ops, NULL);
    jvs_attach(&idz_jvs_io3.jvs);

    return S_OK;
}

static void idz_jvs_read_switches(void *ctx, struct io3_switch_state *out)
{
    uint8_t opbtn;
    uint8_t gamebtn;
    uint8_t gear;

    assert(out != NULL);

    opbtn = 0;
    gamebtn = 0;
    gear = 0;

    idz_io_jvs_read_buttons(&opbtn, &gamebtn);
    idz_io_jvs_read_shifter(&gear);

    /* Update gameplay buttons */

    if (gamebtn & IDZ_IO_GAMEBTN_UP) {
        out->p1 |= 1 << 13;
    }

    if (gamebtn & IDZ_IO_GAMEBTN_DOWN) {
        out->p1 |= 1 << 12;
    }

    if (gamebtn & IDZ_IO_GAMEBTN_LEFT) {
        out->p1 |= 1 << 11;
    }

    if (gamebtn & IDZ_IO_GAMEBTN_RIGHT) {
        out->p1 |= 1 << 10;
    }

    if (gamebtn & IDZ_IO_GAMEBTN_START) {
        out->p1 |= 1 << 15;
    }

    if (gamebtn & IDZ_IO_GAMEBTN_VIEW_CHANGE) {
        out->p1 |= 1 << 9;
    }

    /* Update simulated six-speed shifter */

    if (gear > 6) {
        gear = 6;
    }

    out->p2 = idz_jvs_gear_signals[gear];

    /* Update test/service buttons */

    if (opbtn & IDZ_IO_OPBTN_TEST) {
        out->system = 0x80;
    } else {
        out->system = 0;
    }

    if (opbtn & IDZ_IO_OPBTN_SERVICE) {
        out->p1 |= 1 << 14;
    }
}

static void idz_jvs_read_analogs(
        void *ctx,
        uint16_t *analogs,
        uint8_t nanalogs)
{
    struct idz_io_analog_state state;

    assert(analogs != NULL);

    memset(&state, 0, sizeof(state));
    idz_io_jvs_read_analogs(&state);

    if (nanalogs > 0) {
        analogs[0] = 0x8000 + state.wheel;
    }

    if (nanalogs > 1) {
        analogs[1] = state.accel;
    }

    if (nanalogs > 2) {
        analogs[2] = state.brake;
    }
}

static void idz_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out)
{
    if (slot_no > 0) {
        return;
    }

    idz_io_jvs_read_coin_counter(out);
}
