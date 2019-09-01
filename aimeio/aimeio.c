#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "aimeio/aimeio.h"

#include "util/crc.h"
#include "util/dprintf.h"

struct aime_io_config {
    wchar_t id_path[MAX_PATH];
    uint8_t vk_scan;
};

static struct aime_io_config aime_io_cfg;
static uint8_t aime_io_luid[10];

static void aime_io_config_read(
        struct aime_io_config *cfg,
        const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    GetPrivateProfileStringW(
            L"aime",
            L"cardPath",
            L"DEVICE\\aime.txt",
            cfg->id_path,
            _countof(cfg->id_path),
            filename);

    cfg->vk_scan = GetPrivateProfileIntW(
            L"aime",
            L"scan",
            VK_RETURN,
            filename);

}

HRESULT aime_io_init(void)
{
    aime_io_config_read(&aime_io_cfg, L"segatools.ini");

    return S_OK;
}

void aime_io_fini(void)
{
}

HRESULT aime_io_mifare_poll(uint8_t unit_no, uint32_t *uid)
{
    HRESULT hr;
    FILE *f;
    size_t i;
    int byte;
    int r;

    if (unit_no != 0) {
        return S_FALSE;
    }

    hr = S_FALSE;
    f = NULL;

    if (!(GetAsyncKeyState(aime_io_cfg.vk_scan) & 0x8000)) {
        goto end;
    }

    f = _wfopen(aime_io_cfg.id_path, L"r");

    if (f == NULL) {
        dprintf("Aime DLL: Failed to open %S\n", aime_io_cfg.id_path);

        goto end;
    }

    for (i = 0 ; i < sizeof(aime_io_luid) ; i++) {
        r = fscanf(f, "%02x ", &byte);

        if (r != 1) {
            dprintf("Aime DLL: fscanf[%i] failed: %i\n", (int) i, r);

            goto end;
        }

        aime_io_luid[i] = byte;
    }

    /* NOTE: We are just arbitrarily using the CRC32 of the LUID here, real
       cards do not work like this! However, neither the application code nor
       the network protocol care what the UID is, it just has to be a stable
       unique identifier for over-the-air NFC communications. */

    *uid = crc32(aime_io_luid, sizeof(aime_io_luid), 0);

    hr = S_OK;

end:
    if (f != NULL) {
        fclose(f);
    }

    return hr;
}

HRESULT aime_io_mifare_read_luid(
        uint8_t unit_no,
        uint32_t uid,
        uint8_t *luid,
        size_t luid_size)
{
    assert(luid != NULL);
    assert(luid_size == sizeof(aime_io_luid));

    memcpy(luid, aime_io_luid, luid_size);

    return S_OK;
}

void aime_io_led_set_color(uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b)
{}
