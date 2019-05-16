#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ds_config {
    bool enable;
    uint8_t region;
    wchar_t serial_no[17];
};

struct eeprom_config {
    bool enable;
    wchar_t path[MAX_PATH];
};

struct gpio_config {
    bool enable;
    uint8_t vk_sw1;
    uint8_t vk_sw2;
    bool dipsw[8];
};

struct jvs_config {
    bool enable;
};

struct sram_config {
    bool enable;
    wchar_t path[MAX_PATH];
};

struct amex_config {
    struct ds_config ds;
    struct eeprom_config eeprom;
    struct gpio_config gpio;
    struct jvs_config jvs;
    struct sram_config sram;
};

void ds_config_load(struct ds_config *cfg, const wchar_t *filename);
void eeprom_config_load(struct eeprom_config *cfg, const wchar_t *filename);
void gpio_config_load(struct gpio_config *cfg, const wchar_t *filename);
void jvs_config_load(struct jvs_config *cfg, const wchar_t *filename);
void sram_config_load(struct sram_config *cfg, const wchar_t *filename);
void amex_config_load(struct amex_config *cfg, const wchar_t *filename);
