#include <windows.h>
#include <winsock2.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform/config.h"

void nu_config_load(struct nu_config *cfg,const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    amvideo_config_load(&cfg->amvideo, filename);
    hwmon_config_load(&cfg->hwmon, filename);
    misc_config_load(&cfg->misc, filename);
    nusec_config_load(&cfg->nusec, filename);
    vfs_config_load(&cfg->vfs, filename);
}

void amvideo_config_load(struct amvideo_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"amvideo", L"enable", 1, filename);
}

void hwmon_config_load(struct hwmon_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"hwmon", L"enable", 1, filename);
}

void misc_config_load(struct misc_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);

    cfg->enable = GetPrivateProfileIntW(L"misc", L"enable", 1, filename);
}

void nusec_config_load(struct nusec_config *cfg, const wchar_t *filename)
{
    wchar_t keychip_id[17];
    wchar_t game_id[5];
    wchar_t platform_id[5];
    wchar_t subnet[16];
    int ip[4];
    size_t i;

    assert(cfg != NULL);
    assert(filename != NULL);

    memset(cfg, 0, sizeof(*cfg));
    memset(keychip_id, 0, sizeof(keychip_id));
    memset(game_id, 0, sizeof(game_id));
    memset(platform_id, 0, sizeof(platform_id));
    memset(subnet, 0, sizeof(subnet));

    cfg->enable = GetPrivateProfileIntW(L"keychip", L"enable", 1, filename);

    GetPrivateProfileStringW(
            L"keychip",
            L"id",
            L"A69E-01A88888888",
            keychip_id,
            _countof(keychip_id),
            filename);

    GetPrivateProfileStringW(
            L"keychip",
            L"gameId",
            L"",
            game_id,
            _countof(game_id),
            filename);

    GetPrivateProfileStringW(
            L"keychip",
            L"platformId",
            L"",
            platform_id,
            _countof(platform_id),
            filename);

    cfg->region = GetPrivateProfileIntW(L"keychip", L"region", 1, filename);
    cfg->system_flag = GetPrivateProfileIntW(
            L"keychip",
            L"systemFlag",
            0x64,
            filename);

    GetPrivateProfileStringW(
            L"keychip",
            L"subnet",
            L"0.0.0.0",
            subnet,
            _countof(subnet),
            filename);

    for (i = 0 ; i < 16 ; i++) {
        cfg->keychip_id[i] = (char) keychip_id[i];
    }

    for (i = 0 ; i < 4 ; i++) {
        cfg->game_id[i] = (char) game_id[i];
    }

    for (i = 0 ; i < 4 ; i++) {
        cfg->platform_id[i] = (char) platform_id[i];
    }

    swscanf(subnet, L"%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
    cfg->subnet = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];

    GetPrivateProfileStringW(
            L"keychip",
            L"billingCa",
            L"DEVICE\\ca.crt",
            cfg->billing_ca,
            _countof(cfg->billing_ca),
            filename);

    GetPrivateProfileStringW(
            L"keychip",
            L"billingPub",
            L"DEVICE\\billing.pub",
            cfg->billing_pub,
            _countof(cfg->billing_pub),
            filename);
}

void vfs_config_load(struct vfs_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"vfs", L"enable", 1, filename);

    GetPrivateProfileStringW(
            L"vfs",
            L"amfs",
            L"E:\\",
            cfg->amfs,
            _countof(cfg->amfs),
            filename);

    GetPrivateProfileStringW(
            L"vfs",
            L"appdata",
            L"Y:\\",
            cfg->appdata,
            _countof(cfg->appdata),
            filename);
}

