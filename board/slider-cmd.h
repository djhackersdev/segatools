#pragma once

#include "board/slider-frame.h"

enum {
    SLIDER_CMD_AUTO_SCAN = 0x01,
    SLIDER_CMD_SET_LED = 0x02,
    SLIDER_CMD_AUTO_SCAN_START = 0x03,
    SLIDER_CMD_AUTO_SCAN_STOP = 0x04,
    SLIDER_CMD_DIVA_UNK_09 = 0x09,
    SLIDER_CMD_DIVA_UNK_0A = 0x0A,
    SLIDER_CMD_RESET = 0x10,
    SLIDER_CMD_GET_BOARD_INFO = 0xF0,
};

struct slider_req_set_led {
    struct slider_hdr hdr;
    struct {
        uint8_t unk; /* 0x28, decimal 40. meaning unknown. */
        uint8_t rgb[96];
    } payload;
};

union slider_req_any {
    struct slider_hdr hdr;
    struct slider_req_set_led set_led;
    uint8_t bytes[260];
};

struct slider_resp_get_board_info {
    struct slider_hdr hdr;
    char version[32];
};

struct slider_resp_auto_scan {
    struct slider_hdr hdr;
    uint8_t pressure[32];
};
