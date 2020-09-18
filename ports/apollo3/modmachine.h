#ifndef MICROPY_INCLUDED_APOLLO3_MODMACHINE_H
#define MICROPY_INCLUDED_APOLLO3_MODMACHINE_H

#include "py/obj.h"

// I2C defines
#define AP3_I2C_INTERFACES 2

#define AP3_I2C0_IOM 4 // Secify that I2C-0 uses IOMaster instance 4
#define AP3_I2C1_IOM 3 // Secify that I2C-1 uses IOMaster instance 3

// SPI Defines
#define AP3_SPI_INTERFACES 1

#define AP3_SPI_IOM0 0                   // Specify that SPI uses IOMaster 0
#define AP3_SPI_DUP ap3_spi_full_duplex // Specify that SPI is full-duplex

extern const mp_obj_type_t machine_timer_type;
extern const mp_obj_type_t machine_pin_type;
extern const mp_obj_type_t machine_i2c_type;
extern const mp_obj_type_t machine_rtc_type;

void machine_pins_init(void);
void machine_pins_deinit(void);
void machine_timer_deinit_all(void);

#endif // MICROPY_INCLUDED_APOLLO3_MODMACHINE_H
