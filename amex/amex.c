#include "amex/amex.h"
#include "amex/ds.h"
#include "amex/eeprom.h"
#include "amex/gpio.h"
#include "amex/jvs.h"
#include "amex/sram.h"

void amex_hook_init(void)
{
    ds_hook_init();
    eeprom_hook_init();
    gpio_hook_init();
    jvs_hook_init();
    sram_hook_init();
}
