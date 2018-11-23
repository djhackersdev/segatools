#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board/io3.h"

#include "jvs/jvs-bus.h"

#include "nu/jvs.h"

#include "util/dprintf.h"

static void chunithm_jvs_read_switches(void *ctx, struct io3_switch_state *out);
static uint16_t chunithm_jvs_consume_coins(void *ctx, uint8_t slot_no);

static const struct io3_ops chunithm_jvs_io3_ops = {
    .read_switches  = chunithm_jvs_read_switches,
    .consume_coins  = chunithm_jvs_consume_coins,
};

static struct io3 chunithm_jvs_io3;
static size_t chunithm_jvs_rise_pos;
static bool chunithm_jvs_coin;

void chunithm_jvs_init(void)
{
    io3_init(&chunithm_jvs_io3, NULL, &chunithm_jvs_io3_ops, NULL);
    jvs_attach(&chunithm_jvs_io3.jvs);
}

static void chunithm_jvs_read_switches(void *ctx, struct io3_switch_state *out)
{
    assert(out != NULL);

    /* Update simulated raise/lower state */

    if (GetAsyncKeyState(VK_SPACE)) {
        if (chunithm_jvs_rise_pos < 6) {
            chunithm_jvs_rise_pos++;
        }
    } else {
        if (chunithm_jvs_rise_pos > 0) {
            chunithm_jvs_rise_pos--;
        }
    }

    /* Render the state. Every case falls through, this is intentional. */

    out->p1 = 0;
    out->p2 = 0;

    switch (chunithm_jvs_rise_pos) {
    case 0: out->p2 |= 0x0020;
    case 1: out->p1 |= 0x0020;
    case 2: out->p2 |= 0x0010;
    case 3: out->p1 |= 0x0010;
    case 4: out->p2 |= 0x0008;
    case 5: out->p1 |= 0x0008;
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

static uint16_t chunithm_jvs_consume_coins(void *ctx, uint8_t slot_no)
{
    if (slot_no > 0) {
        return 0;
    }

    if (GetAsyncKeyState('3')) {
        if (chunithm_jvs_coin) {
            return 0;
        } else {
            dprintf("Chunithm JVS: Coin drop\n");
            chunithm_jvs_coin = true;

            return 1;
        }
    } else {
        chunithm_jvs_coin = false;

        return 0;
    }
}
