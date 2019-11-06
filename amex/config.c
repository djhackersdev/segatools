#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "amex/amex.h"
#include "amex/config.h"
#include "amex/ds.h"
#include "amex/eeprom.h"
#include "amex/gpio.h"
#include "amex/jvs.h"
#include "amex/sram.h"

void ds_config_load(struct ds_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"ds", L"enable", 1, filename);
    cfg->region = GetPrivateProfileIntW(L"ds", L"region", 1, filename);

    GetPrivateProfileStringW(
            L"ds",
            L"serialNo",
            L"AAVE-01A99999999",
            cfg->serial_no,
            _countof(cfg->serial_no),
            filename);
}

void eeprom_config_load(struct eeprom_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"eeprom", L"enable", 1, filename);

    GetPrivateProfileStringW(
            L"eeprom",
            L"path",
            L"DEVICE\\eeprom.bin",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void gpio_config_load(struct gpio_config *cfg, const wchar_t *filename)
{
    wchar_t name[7];
    size_t i;

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"gpio", L"enable", 1, filename);
    cfg->vk_sw1 = GetPrivateProfileIntW(L"gpio", L"sw1", VK_F1, filename);
    cfg->vk_sw2 = GetPrivateProfileIntW(L"gpio", L"sw2", VK_F2, filename);

    wcscpy_s(name, _countof(name), L"dipsw0");

    for (i = 0 ; i < 8 ; i++) {
        name[5] = L'1' + i;
        cfg->dipsw[i] = GetPrivateProfileIntW(L"gpio", name, 0, filename);
    }
}

void jvs_config_load(struct jvs_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"jvs", L"enable", 1, filename);
}

void sram_config_load(struct sram_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"sram", L"enable", 1, filename);

    GetPrivateProfileStringW(
            L"sram",
            L"path",
            L"DEVICE\\sram.bin",
            cfg->path,
            _countof(cfg->path),
            filename);
}

void amex_config_load(struct amex_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    ds_config_load(&cfg->ds, filename);
    eeprom_config_load(&cfg->eeprom, filename);
    gpio_config_load(&cfg->gpio, filename);
    jvs_config_load(&cfg->jvs, filename);
    sram_config_load(&cfg->sram, filename);
}
