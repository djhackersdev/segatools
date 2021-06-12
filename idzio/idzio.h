#pragma once

/* INITIAL D ZERO CUSTOM IO API

   This API definition allows custom driver DLLs to be defined for the
   emulation of Initial D Zero cabinets. To be honest, there is very
   little reason to want to do this, since driving game controllers are a
   mostly-standardized PC peripheral which can be adequately controlled by the
   built-in DirectInput and XInput support in idzhook. However, previous
   versions of Segatools broke this functionality out into a separate DLL just
   like all of the other supported games, so in the interests of maintaining
   backwards compatibility we provide the option to load custom IDZIO
   implementations as well. */

#include <windows.h>

#include <stdint.h>

enum {
    IDZ_IO_OPBTN_TEST = 0x01,
    IDZ_IO_OPBTN_SERVICE = 0x02,
};

enum {
    IDZ_IO_GAMEBTN_UP = 0x01,
    IDZ_IO_GAMEBTN_DOWN = 0x02,
    IDZ_IO_GAMEBTN_LEFT = 0x04,
    IDZ_IO_GAMEBTN_RIGHT = 0x08,
    IDZ_IO_GAMEBTN_START = 0x10,
    IDZ_IO_GAMEBTN_VIEW_CHANGE = 0x20,
};

struct idz_io_analog_state {
    /* Current steering wheel position, where zero is the centered position.

       The game will accept any signed 16-bit position value, however a real
       cabinet will report a value of approximately +/- 25230 when the wheel
       is at full lock. Steering wheel positions of a magnitude greater than
       this value are not possible on a real cabinet. */

    int16_t wheel;

    /* Current position of the accelerator pedal, where 0 is released. */

    uint16_t accel;

    /* Current position of the brake pedal, where 0 is released. */

    uint16_t brake;
};

/* Get the version of the IDZ IO API that this DLL supports. This
   function should return a positive 16-bit integer, where the high byte is
   the major version and the low byte is the minor version (as defined by the
   Semantic Versioning standard).

   The latest API version as of this writing is 0x0100. */

uint16_t idz_io_get_api_version(void);

/* Initialize JVS-based input. This function will be called before any other
   idz_io_jvs_*() function calls. Errors returned from this function will
   manifest as a disconnected JVS bus.

   All subsequent calls may originate from arbitrary threads and some may
   overlap with each other. Ensuring synchronization inside your IO DLL is
   your responsibility.

   Minimum API version: 0x0100 */

HRESULT idz_io_jvs_init(void);

/* Poll the current state of the cabinet's JVS analog inputs. See structure
   definition above for details.

   Minimum API version: 0x0100 */

void idz_io_jvs_read_analogs(struct idz_io_analog_state *out);

/* Poll the current state of the cabinet's JVS input buttons and return them
   through the opbtn and gamebtn out parameters. See enum definitions at the
   top of this file for a list of bit masks to be used with these out
   parameters.

   Minimum API version: 0x0100 */

void idz_io_jvs_read_buttons(uint8_t *opbtn, uint8_t *gamebtn);

/* Poll the current position of the six-speed shifter and return it via the
   gear out parameter. Valid values are 0 for neutral and 1-6 for gears 1-6.

   idzhook internally translates this gear position value into the correct
   combination of Gear Left, Gear Right, Gear Up, Gear Down buttons that the
   game will then interpret as the current position of the gear lever.

   Minimum API version: 0x0100 */

void idz_io_jvs_read_shifter(uint8_t *gear);

/* Read the current state of the coin counter. This value should be incremented
   for every coin detected by the coin acceptor mechanism. This count does not
   need to persist beyond the lifetime of the process.

   Minimum API version: 0x0100 */

void idz_io_jvs_read_coin_counter(uint16_t *total);
