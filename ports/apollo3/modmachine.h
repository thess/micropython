#ifndef MICROPY_INCLUDED_APOLLO3_MODMACHINE_H
#define MICROPY_INCLUDED_APOLLO3_MODMACHINE_H

#include "py/obj.h"

#include "mpconfigboard.h"

extern const mp_obj_type_t machine_timer_type;
extern const mp_obj_type_t machine_pin_type;
extern const mp_obj_type_t machine_hw_i2c_type;
extern const mp_obj_type_t machine_hw_spi_type;
extern const mp_obj_type_t machine_rtc_type;

void machine_pins_init(void);
void machine_pins_deinit(void);
void machine_timer_deinit_all(void);

#endif // MICROPY_INCLUDED_APOLLO3_MODMACHINE_H
