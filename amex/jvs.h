#pragma once

#include <windows.h>

#include <stdbool.h>

#include "jvs/jvs-bus.h"

DEFINE_GUID(
        jvs_guid,
        0xDB6BBB45,
        0xCC96,
        0x4288,
        0xAA, 0x00, 0x6C, 0x00, 0xD7, 0x67, 0xBD, 0xBF);

struct jvs_config {
    bool enable;
};

typedef HRESULT (*jvs_provider_t)(struct jvs_node **root);

HRESULT jvs_hook_init(const struct jvs_config *cfg, jvs_provider_t provider);
