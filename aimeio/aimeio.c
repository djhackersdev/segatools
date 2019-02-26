#include <windows.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "aimeio/aimeio.h"

#include "util/crc.h"
#include "util/dprintf.h"

static const char aime_io_path[] = "DEVICE\\aime.txt";

static uint8_t aime_io_luid[10];

HRESULT aime_io_init(void)
{
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

    if (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) {
        goto end;
    }

    f = fopen(aime_io_path, "r");

    if (f == NULL) {
        dprintf("Aime DLL: Failed to open %s\n", aime_io_path);

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
