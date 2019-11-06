#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

struct gpio_config {
    bool enable;
    uint8_t vk_sw1;
    uint8_t vk_sw2;
    bool dipsw[8];
};

DEFINE_GUID(
        gpio_guid,
        0xE9A26688,
        0xF522,
        0x44FA,
        0xBF, 0xEE, 0x59, 0xDD, 0x16, 0x15, 0x56, 0x6C);

HRESULT gpio_hook_init(const struct gpio_config *cfg);
