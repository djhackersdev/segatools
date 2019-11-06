#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

#include "platform/nusec.h"

struct netenv_config {
    bool enable;
    uint8_t addr_suffix;
    uint8_t router_suffix;
    uint8_t mac_addr[6];
};

HRESULT netenv_hook_init(
        const struct netenv_config *cfg,
        const struct nusec_config *kc_cfg);

