#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "idzio/backend.h"
#include "idzio/config.h"
#include "idzio/di.h"
#include "idzio/idzio.h"
#include "idzio/xi.h"

#include "util/dprintf.h"
#include "util/str.h"

static struct idz_io_config idz_io_cfg;
static const struct idz_io_backend *idz_io_backend;
static bool idz_io_coin;
static uint16_t idz_io_coins;

uint16_t idz_io_get_api_version(void)
{
    return 0x0100;
}

HRESULT idz_io_jvs_init(void)
{
    HINSTANCE inst;
    HRESULT hr;

    assert(idz_io_backend == NULL);

    inst = GetModuleHandleW(NULL);

    if (inst == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        dprintf("GetModuleHandleW failed: %lx\n", hr);

        return hr;
    }

    idz_io_config_load(&idz_io_cfg, L".\\segatools.ini");

    if (wstr_ieq(idz_io_cfg.mode, L"dinput")) {
        hr = idz_di_init(&idz_io_cfg.di, inst, &idz_io_backend);
    } else if (wstr_ieq(idz_io_cfg.mode, L"xinput")) {
        hr = idz_xi_init(&idz_io_cfg.xi, &idz_io_backend);
    } else {
        hr = E_INVALIDARG;
        dprintf("IDZ IO: Invalid IO mode \"%S\", use dinput or xinput\n",
                idz_io_cfg.mode);
    }

    return hr;
}

void idz_io_jvs_read_buttons(uint8_t *opbtn_out, uint8_t *gamebtn_out)
{
    uint8_t opbtn;

    assert(idz_io_backend != NULL);
    assert(opbtn_out != NULL);
    assert(gamebtn_out != NULL);

    opbtn = 0;

    if (GetAsyncKeyState(idz_io_cfg.vk_test) & 0x8000) {
        opbtn |= IDZ_IO_OPBTN_TEST;
    }

    if (GetAsyncKeyState(idz_io_cfg.vk_service) & 0x8000) {
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

void idz_io_jvs_read_analogs(struct idz_io_analog_state *out)
{
    struct idz_io_analog_state tmp;

    assert(out != NULL);
    assert(idz_io_backend != NULL);

    idz_io_backend->jvs_read_analogs(&tmp);

    /* Apply steering wheel restriction. Real cabs only report about 77% of
       the IO-3's max ADC output value when the wheel is turned to either of
       its maximum positions. To match this behavior we set the default value
       for the wheel restriction config parameter to 97 (out of 128). This
       scaling factor is applied using fixed-point arithmetic below. */

    out->wheel = (tmp.wheel * idz_io_cfg.restrict_) / 128;
    out->accel = tmp.accel;
    out->brake = tmp.brake;
}

void idz_io_jvs_read_coin_counter(uint16_t *out)
{
    assert(out != NULL);

    /* Coin counter is not backend-specific */

    if (    idz_io_cfg.vk_coin &&
            (GetAsyncKeyState(idz_io_cfg.vk_coin) & 0x8000)) {
        if (!idz_io_coin) {
            idz_io_coin = true;
            idz_io_coins++;
        }
    } else {
        idz_io_coin = false;
    }

    *out = idz_io_coins;
}
