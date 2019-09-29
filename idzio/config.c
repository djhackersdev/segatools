#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "idzio/config.h"

void idz_di_config_load(struct idz_di_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"dinput",
            L"deviceName",
            L"",
            cfg->device_name,
            _countof(cfg->device_name),
            filename);

    GetPrivateProfileStringW(
            L"dinput",
            L"brakeAxis",
            L"RZ",
            cfg->brake_axis,
            _countof(cfg->brake_axis),
            filename);

    GetPrivateProfileStringW(
            L"dinput",
            L"accelAxis",
            L"Y",
            cfg->accel_axis,
            _countof(cfg->accel_axis),
            filename);

    cfg->start = GetPrivateProfileIntW(L"dinput", L"start", 0, filename);
    cfg->view_chg = GetPrivateProfileIntW(L"dinput", L"viewChg", 0, filename);
    cfg->shift_dn = GetPrivateProfileIntW(L"dinput", L"shiftDn", 0, filename);
    cfg->shift_up = GetPrivateProfileIntW(L"dinput", L"shiftUp", 0, filename);
}

void idz_io_config_load(struct idz_io_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->vk_test = GetPrivateProfileIntW(L"io3", L"test", '1', filename);
    cfg->vk_service = GetPrivateProfileIntW(L"io3", L"service", '2', filename);
    cfg->vk_coin = GetPrivateProfileIntW(L"io3", L"coin", '3', filename);

    GetPrivateProfileStringW(
            L"io3",
            L"mode",
            L"xinput",
            cfg->mode,
            _countof(cfg->mode),
            filename);

    idz_shifter_config_load(&cfg->shifter, filename);
    idz_di_config_load(&cfg->di, filename);
}

void idz_shifter_config_load(
        struct idz_shifter_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->auto_neutral = GetPrivateProfileIntW(
            L"io3",
            L"autoNeutral",
            0,
            filename);
}

