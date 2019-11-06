#pragma once

#include <windows.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "amex/amex.h"
#include "amex/ds.h"
#include "amex/eeprom.h"
#include "amex/gpio.h"
#include "amex/jvs.h"
#include "amex/sram.h"

void ds_config_load(struct ds_config *cfg, const wchar_t *filename);
void eeprom_config_load(struct eeprom_config *cfg, const wchar_t *filename);
void gpio_config_load(struct gpio_config *cfg, const wchar_t *filename);
void jvs_config_load(struct jvs_config *cfg, const wchar_t *filename);
void sram_config_load(struct sram_config *cfg, const wchar_t *filename);
void amex_config_load(struct amex_config *cfg, const wchar_t *filename);
