#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aimeio/aimeio.h"

#include "util/crc.h"
#include "util/dprintf.h"

enum {
    AIME_IO_TOUCH_MSEC = 1000,
};

struct aime_io_config {
    wchar_t aime_path[MAX_PATH];
    wchar_t felica_path[MAX_PATH];
    uint8_t vk_scan;
};

static struct aime_io_config aime_io_cfg;
static uint8_t aime_io_aime_id[10];
static uint8_t aime_io_felica_id[8];
static uint32_t aime_io_touch_t;
static bool aime_io_aime_id_present;
static bool aime_io_felica_id_present;
static bool aime_io_latch;

static void aime_io_config_read(
        struct aime_io_config *cfg,
        const wchar_t *filename);

static HRESULT aime_io_read_id_file(
        const wchar_t *path,
        uint8_t *bytes,
        size_t nbytes);

static HRESULT aime_io_nfc_poll_rise(void);
static void aime_io_nfc_poll_fall(void);

static void aime_io_config_read(
        struct aime_io_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"aime",
            L"aimePath",
            L"DEVICE\\aime.txt",
            cfg->aime_path,
            _countof(cfg->aime_path),
            filename);

    GetPrivateProfileStringW(
            L"aime",
            L"felicaPath",
            L"DEVICE\\felica.txt",
            cfg->felica_path,
            _countof(cfg->felica_path),
            filename);

    cfg->vk_scan = GetPrivateProfileIntW(
            L"aime",
            L"scan",
            VK_RETURN,
            filename);
}

static HRESULT aime_io_read_id_file(
        const wchar_t *path,
        uint8_t *bytes,
        size_t nbytes)
{
    HRESULT hr;
    FILE *f;
    size_t i;
    int byte;
    int r;

    f = _wfopen(path, L"r");

    if (f == NULL) {
        return S_FALSE;
    }

    memset(bytes, 0, nbytes);

    for (i = 0 ; i < nbytes ; i++) {
        r = fscanf(f, "%02x ", &byte);

        if (r != 1) {
            hr = E_FAIL;
            dprintf("AimeIO DLL: %S: fscanf[%i] failed: %i\n",
                    path,
                    (int) i,
                    r);

            goto end;
        }

        bytes[i] = byte;
    }

    hr = S_OK;

end:
    if (f != NULL) {
        fclose(f);
    }

    return hr;
}

HRESULT aime_io_init(void)
{
    aime_io_config_read(&aime_io_cfg, L"segatools.ini");

    return S_OK;
}

void aime_io_fini(void)
{
}

HRESULT aime_io_nfc_poll(uint8_t unit_no)
{
    uint32_t dt;
    uint32_t t;
    bool level;
    HRESULT hr;

    if (unit_no != 0) {
        return S_FALSE;
    }

    t = GetTickCount();
    level = GetAsyncKeyState(aime_io_cfg.vk_scan) & 0x8000;

    /* 1. Linger a card for AIME_IO_TOUCH_MSEC after vk_scan has been released.
       2. Detect rising and falling edges and call the relevant worker fns. */

    if (level) {
        aime_io_touch_t = t;
    }

    if (aime_io_latch) {
        dt = aime_io_touch_t - t;

        if (dt < AIME_IO_TOUCH_MSEC) {
            hr = S_OK;
        } else {
            aime_io_nfc_poll_fall();
            aime_io_latch = false;
            hr = S_FALSE;
        }
    } else {
        if (level) {
            hr = aime_io_nfc_poll_rise();
            aime_io_latch = true;
        } else {
            hr = S_FALSE;
        }
    }

    return hr;
}

static HRESULT aime_io_nfc_poll_rise(void)
{
    HRESULT hr;

    hr = aime_io_read_id_file(
            aime_io_cfg.aime_path,
            aime_io_aime_id,
            sizeof(aime_io_aime_id));

    if (SUCCEEDED(hr) && hr != S_FALSE) {
        aime_io_aime_id_present = true;

        return hr;
    }

    hr = aime_io_read_id_file(
            aime_io_cfg.felica_path,
            aime_io_felica_id,
            sizeof(aime_io_felica_id));

    if (SUCCEEDED(hr) && hr != S_FALSE) {
        aime_io_felica_id_present = true;

        return hr;
    }

    return S_FALSE;
}

static void aime_io_nfc_poll_fall(void)
{
    aime_io_aime_id_present = false;
    aime_io_felica_id_present = false;
}

HRESULT aime_io_nfc_get_aime_id(
        uint8_t unit_no,
        uint8_t *luid,
        size_t luid_size)
{
    assert(luid != NULL);
    assert(luid_size == sizeof(aime_io_aime_id));

    if (unit_no != 0 || !aime_io_aime_id_present) {
        return S_FALSE;
    }

    memcpy(luid, aime_io_aime_id, luid_size);

    return S_OK;
}

HRESULT aime_io_nfc_get_felica_id(uint8_t unit_no, uint64_t *IDm)
{
    uint64_t val;
    size_t i;

    assert(IDm != NULL);

    if (unit_no != 0 || !aime_io_felica_id_present) {
        return S_FALSE;
    }

    val = 0;

    for (i = 0 ; i < 8 ; i++) {
        val = (val << 8) | aime_io_felica_id[i];
    }

    *IDm = val;

    return S_OK;
}

void aime_io_led_set_color(uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b)
{}
