#pragma once

#include <stdint.h>

#include "board/sg-cmd.h"

enum {
    SG_RGB_CMD_SET_COLOR            = 0x81,
    SG_RGB_CMD_RESET                = 0xF5,
    SG_RGB_CMD_GET_INFO             = 0xF0,
};

struct sg_led_res_reset {
    struct sg_res_header res;
    uint8_t payload;
};

struct sg_led_res_get_info {
    struct sg_res_header res;
    uint8_t payload[9];
};

struct sg_led_req_set_color {
    struct sg_req_header req;
    uint8_t payload[3];
};

union sg_led_req_any {
    uint8_t bytes[256];
    struct sg_req_header simple;
    struct sg_led_req_set_color set_color;
};

union sg_led_res_any {
    uint8_t bytes[256];
    struct sg_res_header simple;
    struct sg_led_res_reset reset;
    struct sg_led_res_get_info get_info;
};
