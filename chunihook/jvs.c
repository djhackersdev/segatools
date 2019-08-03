#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "amex/jvs.h"

#include "board/io3.h"

#include "chuniio/chuniio.h"

#include "jvs/jvs-bus.h"

#include "util/dprintf.h"

struct chunithm_jvs_ir_mask {
    uint16_t p1;
    uint16_t p2;
};

static void chunithm_jvs_read_switches(void *ctx, struct io3_switch_state *out);
static void chunithm_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out);

static const struct io3_ops chunithm_jvs_io3_ops = {
    .read_switches      = chunithm_jvs_read_switches,
    .read_coin_counter  = chunithm_jvs_read_coin_counter,
};

static const struct chunithm_jvs_ir_mask chunithm_jvs_ir_masks[] = {
    { 0x0000, 0x0020 },
    { 0x0020, 0x0000 },
    { 0x0000, 0x0010 },
    { 0x0010, 0x0000 },
    { 0x0000, 0x0008 },
    { 0x0008, 0x0000 },
};

static struct io3 chunithm_jvs_io3;

void chunithm_jvs_init(void)
{
    io3_init(&chunithm_jvs_io3, NULL, &chunithm_jvs_io3_ops, NULL);
    jvs_attach(&chunithm_jvs_io3.jvs);
}

static void chunithm_jvs_read_switches(void *ctx, struct io3_switch_state *out)
{
    uint8_t opbtn;
    uint8_t beams;
    size_t i;

    assert(out != NULL);

    opbtn = 0;
    beams = 0;

    chuni_io_jvs_poll(&opbtn, &beams);

    out->system = 0x00;
    out->p1 = 0x0000;
    out->p2 = 0x0000;

    if (opbtn & 0x01) {
        out->system = 0x80;
    } else {
        out->system = 0x00;
    }

    if (opbtn & 0x02) {
        out->p1 |= 0x4000;
    }

    for (i = 0 ; i < 6 ; i++) {
        /* Beam "press" is active-low hence the ~ */
        if (~beams & (1 << i)) {
            out->p1 |= chunithm_jvs_ir_masks[i].p1;
            out->p2 |= chunithm_jvs_ir_masks[i].p2;
        }
    }
}

static void chunithm_jvs_read_coin_counter(
        void *ctx,
        uint8_t slot_no,
        uint16_t *out)
{
    assert(out != NULL);

    if (slot_no > 0) {
        return;
    }

    chuni_io_jvs_read_coin_counter(out);
}
