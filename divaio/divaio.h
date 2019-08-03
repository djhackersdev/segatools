#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

/* Initialize the Project Diva IO provider DLL. This is the first function to be
   called on this DLL. Returning failure from this function will cause the main
   application to immediately exit.

   All subsequent calls may originate from arbitrary threads and some may
   overlap with each other. Ensuring synchronization inside your IO DLL is
   your responsibility. */

HRESULT diva_io_init(void);

/* Poll JVS input.

   opbtn returns the cabinet test/service state, where bit 0 is Test and Bit 1
   is Service.

   gamebtn bits, from least significant to most significant, are:

   Circle Cross Square Triangle Start UNUSED UNUSED UNUSED */

void diva_io_jvs_poll(uint8_t *opbtn, uint8_t *gamebtn);

/* Read the current state of the coin counter. This value should be incremented
   for every coin detected by the coin acceptor mechanism. This count does not
   need to persist beyond the lifetime of the process. */

void diva_io_jvs_read_coin_counter(uint16_t *out);

/* Set the state of the coin blocker. Parameter is true if the blocker is
   disengaged (i.e. coins can be inserted) and false if the blocker is engaged
   (i.e. the coin slot should be physically blocked). */

void diva_io_jvs_set_coin_blocker(bool open);

/* Project Diva touch sliders consist of 32 pressure sensitive cells, where
   cell 1 (array index 0) is the rightmost cell and cell 32 (array index 31) is
   the leftmost cell. */

/* Callback function supplied to your IO DLL. This must be called with a
   pointer to a 32-byte array of pressure values, one byte per slider cell.
   Cells reporting a pressure value of at least 20 are considered to be pressed.
   This threshold is not configurable.

   The callback will copy the pressure state data out of your buffer before
   returning. The pointer will not be retained. */

typedef void (*diva_io_slider_callback_t)(const uint8_t *state);

/* Start polling the slider. Your DLL must start a polling thread and call the
   supplied function periodically from that thread with new input state. The
   update interval is up to you, but if your input device doesn't have any
   preferred interval then 1 kHz is a reasonable maximum frequency.

   Note that you do have to have to call the callback "occasionally" even if
   nothing is changing, otherwise the game will raise a comm timeout error. */

void diva_io_slider_start(diva_io_slider_callback_t callback);

/* Stop polling the slider. You must cease to invoke the input callback before
   returning from this function.

   This *will* be called in the course of regular operation. For example,
   every time you go into the operator menu the slider and all of the other I/O
   on the cabinet gets restarted.

   Following on from the above, the slider polling loop *will* be restarted
   after being stopped in the course of regular operation. Do not permanently
   tear down your input driver in response to this function call. */

void diva_io_slider_stop(void);

/* Update the RGB lighting on the slider. A pointer to an array of 32 * 3 = 96
   bytes is supplied. Layout is probably strictly linear but still TBD. */

void diva_io_slider_set_leds(const uint8_t *rgb);
