#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct nusec_config {
    bool enable;
    char keychip_id[16];
    char game_id[4];
    char platform_id[4];
    uint8_t region;
    uint8_t system_flag;
    uint32_t subnet;
    wchar_t billing_ca[MAX_PATH];
    wchar_t billing_pub[MAX_PATH];
};

HRESULT nusec_hook_init(
        const struct nusec_config *cfg,
        const char *game_id,
        const char *platform_id);
