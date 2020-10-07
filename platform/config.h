#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/amvideo.h"
#include "platform/clock.h"
#include "platform/dns.h"
#include "platform/hwmon.h"
#include "platform/hwreset.h"
#include "platform/misc.h"
#include "platform/netenv.h"
#include "platform/nusec.h"
#include "platform/pcbid.h"
#include "platform/platform.h"
#include "platform/vfs.h"

void platform_config_load(
        struct platform_config *cfg,
        const wchar_t *filename);

void amvideo_config_load(struct amvideo_config *cfg, const wchar_t *filename);
void clock_config_load(struct clock_config *cfg, const wchar_t *filename);
void dns_config_load(struct dns_config *cfg, const wchar_t *filename);
void hwmon_config_load(struct hwmon_config *cfg, const wchar_t *filename);
void hwreset_config_load(struct hwreset_config *cfg, const wchar_t *filename);
void misc_config_load(struct misc_config *cfg, const wchar_t *filename);
void netenv_config_load(struct netenv_config *cfg, const wchar_t *filename);
void nusec_config_load(struct nusec_config *cfg, const wchar_t *filename);
void pcbid_config_load(struct pcbid_config *cfg, const wchar_t *filename);
void vfs_config_load(struct vfs_config *cfg, const wchar_t *filename);
