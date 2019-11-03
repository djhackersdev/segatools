#include <windows.h>

#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#include "chuniio/chuniio.h"

static unsigned int __stdcall chuni_io_slider_thread_proc(void *ctx);

static const int chuni_io_slider_keys[] = {
    'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
};

static bool chuni_io_coin;
static uint16_t chuni_io_coins;
static uint8_t chuni_io_hand_pos;
static HANDLE chuni_io_slider_thread;
static bool chuni_io_slider_stop_flag;

HRESULT chuni_io_jvs_init(void)
{
    return S_OK;
}

void chuni_io_jvs_read_coin_counter(uint16_t *out)
{
    if (out == NULL) {
        return;
    }

    if (GetAsyncKeyState('3')) {
        if (!chuni_io_coin) {
            chuni_io_coin = true;
            chuni_io_coins++;
        }
    } else {
        chuni_io_coin = false;
    }

    *out = chuni_io_coins;
}

void chuni_io_jvs_poll(uint8_t *opbtn, uint8_t *beams)
{
    size_t i;

    if (GetAsyncKeyState('1')) {
        *opbtn |= 0x01; /* Test */
    }

    if (GetAsyncKeyState('2')) {
        *opbtn |= 0x02; /* Service */
    }

    if (GetAsyncKeyState(VK_SPACE)) {
        if (chuni_io_hand_pos < 6) {
            chuni_io_hand_pos++;
        }
    } else {
        if (chuni_io_hand_pos > 0) {
            chuni_io_hand_pos--;
        }
    }

    for (i = 0 ; i < 6 ; i++) {
        if (chuni_io_hand_pos > i) {
            *beams |= (1 << i);
        }
    }
}

void chuni_io_jvs_set_coin_blocker(bool open)
{}

HRESULT chuni_io_slider_init(void)
{
    return S_OK;
}

void chuni_io_slider_start(chuni_io_slider_callback_t callback)
{
    if (chuni_io_slider_thread != NULL) {
        return;
    }

    chuni_io_slider_thread = (HANDLE) _beginthreadex(
            NULL,
            0,
            chuni_io_slider_thread_proc,
            callback,
            0,
            NULL);
}

void chuni_io_slider_stop(void)
{
    if (chuni_io_slider_thread == NULL) {
        return;
    }

    chuni_io_slider_stop_flag = true;

    WaitForSingleObject(chuni_io_slider_thread, INFINITE);
    CloseHandle(chuni_io_slider_thread);
    chuni_io_slider_thread = NULL;
    chuni_io_slider_stop_flag = false;
}

void chuni_io_slider_set_leds(const uint8_t *rgb)
{
}

static unsigned int __stdcall chuni_io_slider_thread_proc(void *ctx)
{
    chuni_io_slider_callback_t callback;
    uint8_t pressure_val;
    uint8_t pressure[32];
    size_t i;

    callback = ctx;

    while (!chuni_io_slider_stop_flag) {
        for (i = 0 ; i < 8 ; i++) {
            if (GetAsyncKeyState(chuni_io_slider_keys[i]) & 0x8000) {
                pressure_val = 20;
            } else {
                pressure_val = 0;
            }

            memset(&pressure[28 - 4 * i], pressure_val, 4);
        }

        callback(pressure);
        Sleep(1);
    }

    return 0;
}
