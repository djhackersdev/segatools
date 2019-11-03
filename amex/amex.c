#include <windows.h>

#include "amex/amex.h"
#include "amex/ds.h"
#include "amex/eeprom.h"
#include "amex/gpio.h"
#include "amex/jvs.h"
#include "amex/sram.h"

#include <assert.h>

HRESULT amex_hook_init(const struct amex_config *cfg, jvs_provider_t jvs)
{
    HRESULT hr;

    assert(cfg != NULL);

    hr = ds_hook_init(&cfg->ds);

    if (FAILED(hr)) {
        return hr;
    }

    hr = eeprom_hook_init(&cfg->eeprom);

    if (FAILED(hr)) {
        return hr;
    }

    hr = gpio_hook_init(&cfg->gpio);

    if (FAILED(hr)) {
        return hr;
    }

    hr = jvs_hook_init(&cfg->jvs, jvs);

    if (FAILED(hr)) {
        return hr;
    }

    hr = sram_hook_init(&cfg->sram);

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}
