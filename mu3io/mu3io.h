#pragma once

#include <windows.h>

#include <stdint.h>

enum {
    MU3_IO_OPBTN_TEST = 0x01,
    MU3_IO_OPBTN_SERVICE = 0x02,
};

enum {
    MU3_IO_GAMEBTN_1 = 0x01,
    MU3_IO_GAMEBTN_2 = 0x02,
    MU3_IO_GAMEBTN_3 = 0x04,
    MU3_IO_GAMEBTN_SIDE = 0x08,
    MU3_IO_GAMEBTN_MENU = 0x10,
};

/* Get the version of the Ongeki IO API that this DLL supports. This
   function should return a positive 16-bit integer, where the high byte is
   the major version and the low byte is the minor version (as defined by the
   Semantic Versioning standard).

   The latest API version as of this writing is 0x0100. */

uint16_t mu3_io_get_api_version(void);

/* Initialize the IO DLL. This is the second function that will be called on
   your DLL, after mu3_io_get_api_version.

   All subsequent calls to this API may originate from arbitrary threads.

   Minimum API version: 0x0100 */

HRESULT mu3_io_init(void);

/* Send any queued outputs (of which there are currently none, though this may
   change in subsequent API versions) and retrieve any new inputs.

   Minimum API version: 0x0100 */

HRESULT mu3_io_poll(void);

/* Get the state of the cabinet's operator buttons as of the last poll. See
   MU3_IO_OPBTN enum above: this contains bit mask definitions for button
   states returned in *opbtn. All buttons are active-high.

   Minimum API version: 0x0100 */

void mu3_io_get_opbtns(uint8_t *opbtn);

/* Get the state of the cabinet's gameplay buttons as of the last poll. See
   MU3_IO_GAMEBTN enum above for bit mask definitions. Inputs are split into
   a left hand side set of inputs and a right hand side set of inputs: the bit
   mappings are the same in both cases.

   All buttons are active-high, even though some buttons' electrical signals
   on a real cabinet are active-low.

   Minimum API version: 0x0100 */

void mu3_io_get_gamebtns(uint8_t *left, uint8_t *right);

/* Get the position of the cabinet lever as of the last poll. The center
   position should be equal to or close to zero.

   The operator will be required to calibrate the lever's range of motion on
   first power-on, so the lever position reported through this API does not
   need to perfectly centered or cover every single position value possible,
   but it should be reasonably close in order to make things easier for the
   operator.

   The calibration screen displays the leftmost and rightmost position signal
   returned from the cabinet's ADC encoder as a pair of raw two's complement
   hexadecimal values. On a real cabinet these leftmost and rightmost
   positions are somewhere around 0xB000 and 0x5000 respectively (remember
   that negative values i.e. left positions have a high most-significant bit),
   although these values can easily vary by +/- 0x1000 across different
   cabinets.

   Minimum API version: 0x0100 */

void mu3_io_get_lever(int16_t *pos);
