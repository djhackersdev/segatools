#include <windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "amex/jvs.h"

#include "board/io3.h"

#include "jvs/jvs-bus.h"

#include "util/dprintf.h"

static void diva_jvs_read_switches(void *ctx, struct io3_switch_state *out);
static uint16_t diva_jvs_read_coin_counter(void *ctx, uint8_t slot_no);

static const struct io3_ops diva_jvs_io3_ops = {
    .read_switches      = diva_jvs_read_switches,
    .read_coin_counter  = diva_jvs_read_coin_counter,
};

static struct io3 diva_jvs_io3;
static bool diva_jvs_coin;
static uint16_t diva_jvs_coins;

void diva_jvs_init(void)
{
    io3_init(&diva_jvs_io3, NULL, &diva_jvs_io3_ops, NULL);
    jvs_attach(&diva_jvs_io3.jvs);
}

static void diva_jvs_read_switches(void *ctx, struct io3_switch_state *out)
{
    assert(out != NULL);

    /* Update gameplay buttons (P2 JVS input is not even polled) */

    if (GetAsyncKeyState('S')) {
        out->p1 |= 1 << 9;
    }

    if (GetAsyncKeyState('F')) {
        out->p1 |= 1 << 8;
    }

    if (GetAsyncKeyState('J')) {
        out->p1 |= 1 << 7;
    }

    if (GetAsyncKeyState('L')) {
        out->p1 |= 1 << 6;
    }

    /* Update start button */

    if (GetAsyncKeyState(VK_SPACE)) {
        out->p1 |= 1 << 15;
    }

    /* Update test/service buttons */

    if (GetAsyncKeyState('1')) {
        out->system = 0x80;
    } else {
        out->system = 0;
    }

    if (GetAsyncKeyState('2')) {
        out->p1 |= 0x4000;
    }
}

static uint16_t diva_jvs_read_coin_counter(void *ctx, uint8_t slot_no)
{
    if (slot_no > 0) {
        return 0;
    }

    if (GetAsyncKeyState('3')) {
        if (!diva_jvs_coin) {
            dprintf("Diva JVS: Coin drop\n");
            diva_jvs_coin = true;
            diva_jvs_coins++;
        }
    } else {
        diva_jvs_coin = false;
    }

    return diva_jvs_coins;
}
