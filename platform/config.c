#include <windows.h>
#include <winsock2.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/amvideo.h"
#include "platform/clock.h"
#include "platform/config.h"
#include "platform/dns.h"
#include "platform/hwmon.h"
#include "platform/hwreset.h"
#include "platform/misc.h"
#include "platform/netenv.h"
#include "platform/nusec.h"
#include "platform/pcbid.h"
#include "platform/platform.h"
#include "platform/vfs.h"

void platform_config_load(struct platform_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    amvideo_config_load(&cfg->amvideo, filename);
    clock_config_load(&cfg->clock, filename);
    dns_config_load(&cfg->dns, filename);
    hwmon_config_load(&cfg->hwmon, filename);
    hwreset_config_load(&cfg->hwreset, filename);
    misc_config_load(&cfg->misc, filename);
    pcbid_config_load(&cfg->pcbid, filename);
    netenv_config_load(&cfg->netenv, filename);
    nusec_config_load(&cfg->nusec, filename);
    vfs_config_load(&cfg->vfs, filename);
}

void amvideo_config_load(struct amvideo_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"amvideo", L"enable", 1, filename);
}

void clock_config_load(struct clock_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->timezone = GetPrivateProfileIntW(L"clock", L"timezone", 1, filename);
    cfg->timewarp = GetPrivateProfileIntW(L"clock", L"timewarp", 0, filename);
    cfg->writeable = GetPrivateProfileIntW(
            L"clock",
            L"writeable",
            0,
            filename);
}

void dns_config_load(struct dns_config *cfg, const wchar_t *filename)
{
    wchar_t default_[128];

    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"dns", L"enable", 1, filename);

    GetPrivateProfileStringW(
            L"dns",
            L"default",
            L"localhost",
            default_,
            _countof(default_),
            filename);

    GetPrivateProfileStringW(
            L"dns",
            L"router",
            default_,
            cfg->router,
            _countof(cfg->router),
            filename);

    GetPrivateProfileStringW(
            L"dns",
            L"startup",
            default_,
            cfg->startup,
            _countof(cfg->startup),
            filename);

    GetPrivateProfileStringW(
            L"dns",
            L"billing",
            default_,
            cfg->billing,
            _countof(cfg->billing),
            filename);

    GetPrivateProfileStringW(
            L"dns",
            L"aimedb",
            default_,
            cfg->aimedb,
            _countof(cfg->aimedb),
            filename);
}

void hwmon_config_load(struct hwmon_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"hwmon", L"enable", 1, filename);
}

void hwreset_config_load(struct hwreset_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"hwreset", L"enable", 1, filename);
}

void misc_config_load(struct misc_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"misc", L"enable", 1, filename);
}

void netenv_config_load(struct netenv_config *cfg, const wchar_t *filename)
{
    wchar_t mac_addr[18];

    assert(cfg != NULL);
    assert(filename != NULL);

    memset(cfg, 0, sizeof(*cfg));

    cfg->enable = GetPrivateProfileIntW(L"netenv", L"enable", 0, filename);

    cfg->addr_suffix = GetPrivateProfileIntW(
            L"netenv",
            L"addrSuffix",
            11,
            filename);

    cfg->router_suffix = GetPrivateProfileIntW(
            L"netenv",
            L"routerSuffix",
            254,
            filename);

    GetPrivateProfileStringW(
            L"netenv",
            L"macAddr",
            L"01:02:03:04:05:06",
            mac_addr,
            _countof(mac_addr),
            filename);

    swscanf(mac_addr,
            L"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &cfg->mac_addr[0],
            &cfg->mac_addr[1],
            &cfg->mac_addr[2],
            &cfg->mac_addr[3],
            &cfg->mac_addr[4],
            &cfg->mac_addr[5],
            &cfg->mac_addr[6]);
}

void nusec_config_load(struct nusec_config *cfg, const wchar_t *filename)
{
    wchar_t keychip_id[17];
    wchar_t game_id[5];
    wchar_t platform_id[5];
    wchar_t subnet[16];
    unsigned int ip[4];
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
            L"192.168.100.0",
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

    swscanf(subnet, L"%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
    cfg->subnet = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | 0;

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

void pcbid_config_load(struct pcbid_config *cfg, const wchar_t *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->enable = GetPrivateProfileIntW(L"pcbid", L"enable", 1, filename);

    GetPrivateProfileStringW(
            L"pcbid",
            L"serialNo",
            L"ACAE01A99999999",
            cfg->serial_no,
            _countof(cfg->serial_no),
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
            L"",
            cfg->amfs,
            _countof(cfg->amfs),
            filename);

    GetPrivateProfileStringW(
            L"vfs",
            L"appdata",
            L"",
            cfg->appdata,
            _countof(cfg->appdata),
            filename);

    GetPrivateProfileStringW(
            L"vfs",
            L"option",
            L"",
            cfg->option,
            _countof(cfg->option),
            filename);
}

