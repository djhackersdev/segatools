#pragma once

#include <windows.h>

#include "jvs/jvs-bus.h"

DEFINE_GUID(
        jvs_guid,
        0xDB6BBB45,
        0xCC96,
        0x4288,
        0xAA, 0x00, 0x6C, 0x00, 0xD7, 0x67, 0xBD, 0xBF);

void jvs_hook_init(void);
void jvs_attach(struct jvs_node *root);
