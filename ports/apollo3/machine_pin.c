/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "am_mcu_apollo.h"
#include "am_hal_gpio.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modmachine.h"
#include "extmod/virtpin.h"

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    uint8_t id;
} machine_pin_obj_t;

typedef struct _machine_pin_irq_obj_t {
    mp_obj_base_t base;
    uint8_t id;
    uint8_t mode;
} machine_pin_irq_obj_t;

const am_hal_gpio_pincfg_t AP3_GPIO_OPENDRAIN_DEFAULT =
{
    .uFuncSel       = 3,
    .ePullup        = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPInput       = AM_HAL_GPIO_PIN_INPUT_ENABLE,         // Not automagic (fnSel:=3)
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN
};

const uint8_t ap3_pin2pad[] = {
    //~ := PWM, A := ADC
    // <Apollo3 Pad>  Board Pin - functions
    25,               //0     - ~RX1/SDA2/MISO2
    24,               //1     - ~TX1/32kHz/SWO
    44,               //2     - ~MOSI4
    35,               //3/A6  - ~A/TX1/I2SDAT/PDMCLK
     4,               //4     - ~RX1/SLINT
    22,               //5     - ~PDMCLK/SWO
    23,               //6     - ~I2SWCLK/CMPOUT
    27,               //7     - ~SCL2/SCK2
    28,               //8     - ~MOSI2/I2SWCLK
    32,               //9/A7  - ~A/SCCIO
    14,               //10    -  SWCH/ADCD1N/TX1/PDMCLK
     7,               //11    - ~MOSI0/CLKOUT
     6,               //12    - ~MISO0/SDA0/I2SDAT
     5,               //13    - ~SCK0/SCL0
    40,               //14    -  SDA4/MISO4/RX1
    39,               //15    - ~SCL4/SCK4/TX1
    43,               //16    - ~SDA3/MISO3/RX1
    42,               //17    - ~SCL3/SCK3/TX1
    26,               //18    - ~LED/SCCRST
    33,               //19/A0 - ~A/SWO/32KHZ
    13,               //20/A1 - ~A/I2SBCLK/RX1
    11,               //21/A2 - ~A/PDMDATA
    29,               //22/A3 - ~A/PDMDATA
    12,               //23/A4 - ~A/PDMCLK/TX1
    31,               //24/A5 - ~A/SCCCLK
};

STATIC const machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type}, 0},    //25     - ~RX1/SDA2/MISO2
    {{&machine_pin_type}, 1},    //24     - ~TX1/32kHz/SWO
    {{&machine_pin_type}, 2},    //44     - ~MOSI4
    {{&machine_pin_type}, 3},    //35/A6  - ~A/TX1/I2SDAT/PDMCLK
    {{&machine_pin_type}, 4},    //4      - ~RX1/SLINT
    {{&machine_pin_type}, 5},    //22     - ~PDMCLK/SWO
    {{&machine_pin_type}, 6},    //23     - ~I2SWCLK/CMPOUT
    {{&machine_pin_type}, 7},    //27     - ~SCL2/SCK2
    {{&machine_pin_type}, 8},    //29     - ~MOSI2/I2SWCLK
    {{&machine_pin_type}, 9},    //32/A7  - ~A/SCCIO
    {{&machine_pin_type}, 10},   //14     -  SWCH/ADCD1N/TX1/PDMCLK
    {{&machine_pin_type}, 11},   //7      - ~MOSI0/CLKOUT
    {{&machine_pin_type}, 12},   //6      - ~MISO0/SDA0/I2SDAT
    {{&machine_pin_type}, 13},   //5      - ~SCK0/SCL0
    {{&machine_pin_type}, 14},   //40     -  SDA4/MISO4/RX1
    {{&machine_pin_type}, 15},   //39     - ~SCL4/SCK4/TX1
    {{&machine_pin_type}, 16},   //43     - ~SDA3/MISO3/RX1
    {{&machine_pin_type}, 17},   //42     - ~SCL3/SCK3/TX1
    {{&machine_pin_type}, 18},   //26     - ~LED/SCCRST
    {{&machine_pin_type}, 19},   //33/A0  - ~A/SWO/32KHZ
    {{&machine_pin_type}, 20},   //13/A1  - ~A/I2SBCLK/RX1
    {{&machine_pin_type}, 21},   //11/A2  - ~A/PDMDATA
    {{&machine_pin_type}, 22},   //29/A3  - ~A/PDMDATA
    {{&machine_pin_type}, 23},   //12/A4  - ~A/PDMCLK/TX1
    {{&machine_pin_type}, 24},   //31/A5  - ~A/SCCCLK
};

// Table of active ISR objects
STATIC machine_pin_irq_obj_t machine_pin_irq_object[AP3_MAX_GPIO_ISR] = {NULL};

void machine_pins_init(void) {
    // Just de-init (reset state)
    machine_pins_deinit();
    memset(&MP_STATE_PORT(machine_pin_irq_handler[0]), 0, sizeof(MP_STATE_PORT(machine_pin_irq_handler)));
    NVIC_SetPriority(GPIO_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
}

void machine_pins_deinit(void) {
    NVIC_DisableIRQ(GPIO_IRQn);
    for (uint8_t pin = 0; pin < MP_ARRAY_SIZE(machine_pin_obj); pin++) {
        // Reset pin (skip uart pins)
        uint8_t pad = ap3_pin2pad[pin];
        am_hal_gpio_pinconfig(pad, g_AM_HAL_GPIO_DISABLE);
        // ... and interrupt masks
        AM_HAL_GPIO_MASKCREATE(IntMask);
        AM_HAL_GPIO_MASKBIT(pIntMask, pad);
        am_hal_gpio_interrupt_disable(pIntMask);
        am_hal_gpio_interrupt_clear(pIntMask);
    }
}

uint8_t machine_pin_get_id(mp_obj_t pin_in) {
    if (mp_obj_get_type(pin_in) != &machine_pin_type) {
        mp_raise_ValueError(MP_ERROR_TEXT("expecting a pin"));
    }
    machine_pin_obj_t *self = pin_in;
    return ap3_pin2pad[self->id];
}

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "Pin(%u)", self->id);
}

enum { AP3_GPIO_OUTPUT, AP3_GPIO_INPUT, AP3_GPIO_TRISTATE };
// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value, ARG_drive };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = AP3_GPIO_OUTPUT}},
        { MP_QSTR_pull, MP_ARG_INT, {.u_int = AM_HAL_GPIO_PIN_PULLUP_NONE}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        { MP_QSTR_drive, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA}},
    };
    static const am_hal_gpio_pincfg_t *ap3_gpio_modes[] = {
        &g_AM_HAL_GPIO_OUTPUT_WITH_READ,
        &g_AM_HAL_GPIO_INPUT,
        &g_AM_HAL_GPIO_TRISTATE        
    };
                        
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // configure/validate mode
    int mode = args[ARG_mode].u_int;
    if (mode < 0 || mode > 2) {
         mp_raise_ValueError(MP_ERROR_TEXT("invalid PIN mode"));
    }
    
    am_hal_gpio_pincfg_t pinconfig = *ap3_gpio_modes[mode];
    // Input: configure pull
    if (mode == AP3_GPIO_INPUT) {
        pinconfig = g_AM_HAL_GPIO_INPUT_PULLUP;
        pinconfig.ePullup = args[ARG_pull].u_int;
    }
    // Output: configure current 
    if (mode == AP3_GPIO_OUTPUT) {
        pinconfig.eDriveStrength = args[ARG_drive].u_int;
    }

    // configure the pin as requested
    if (am_hal_gpio_pinconfig(ap3_pin2pad[self->id], pinconfig) != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("PIN config error"));
    }
 
    // set initial value if requested
    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
       mp_hal_pin_write(ap3_pin2pad[self->id], mp_obj_is_true(args[ARG_value].u_obj));
    }

    return mp_const_none;
}

// constructor(id, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    int wanted_pin = mp_obj_get_int(args[0]);
    const machine_pin_obj_t *self = NULL;
    if (0 <= wanted_pin && wanted_pin < MP_ARRAY_SIZE(machine_pin_obj)) {
        self = (machine_pin_obj_t *)&machine_pin_obj[wanted_pin];
    }
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    if (n_args == 0) {
        // get pin
        return MP_OBJ_NEW_SMALL_INT(mp_hal_pin_read(ap3_pin2pad[self->id]));
    } else {
        mp_hal_pin_write(ap3_pin2pad[self->id], mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// pin.off()
STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    am_hal_gpio_output_clear(ap3_pin2pad[self->id]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

// pin.on()
STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    am_hal_gpio_output_set(ap3_pin2pad[self->id]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

// Forward ref
STATIC mp_obj_t machine_pin_irq();
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, machine_pin_irq);

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_pin_irq_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(AP3_GPIO_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(AP3_GPIO_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(AP3_GPIO_TRISTATE) },

    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(AM_HAL_GPIO_PIN_PULLUP_WEAK) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP15), MP_ROM_INT(AM_HAL_GPIO_PIN_PULLUP_1_5K) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(AM_HAL_GPIO_PIN_PULLDOWN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(AM_HAL_GPIO_PIN_PULLUP_NONE) },

    { MP_ROM_QSTR(MP_QSTR_LOW_POWER), MP_ROM_INT(AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA) },
    { MP_ROM_QSTR(MP_QSTR_MED_POWER), MP_ROM_INT(AM_HAL_GPIO_PIN_DRIVESTRENGTH_8MA) },
    { MP_ROM_QSTR(MP_QSTR_HIGH_POWER), MP_ROM_INT(AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(AM_HAL_GPIO_PIN_INTDIR_LO2HI) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(AM_HAL_GPIO_PIN_INTDIR_HI2LO) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_LEVEL), MP_ROM_INT(AM_HAL_GPIO_PIN_INTDIR_BOTH) },
};

STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ:
            return mp_hal_pin_read(ap3_pin2pad[self->id]);

        case MP_PIN_WRITE:
            mp_hal_pin_write(ap3_pin2pad[self->id], arg);
            return 0;
    }
    return -1;
}

STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

STATIC const mp_pin_p_t pin_pin_p = {
    .ioctl = pin_ioctl,
};

const mp_obj_type_t machine_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = mp_pin_make_new,
    .call = machine_pin_call,
    .protocol = &pin_pin_p,
    .locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};

//*****************************************************************************
//
// Interrupt handler for the GPIO pins.
//
//*****************************************************************************
void
am_gpio_isr(void)
{
    //
    // Read and clear the GPIO interrupt status.
    //
    uint64_t ui64Status;

    am_hal_gpio_interrupt_status_get(false, &ui64Status);
    am_hal_gpio_interrupt_clear(ui64Status);
    am_hal_gpio_interrupt_service(ui64Status);
}

STATIC void machine_pin_isr_handler(void *arg) {
    machine_pin_obj_t *self = arg;
    mp_obj_t handler = MP_STATE_PORT(machine_pin_irq_handler)[self->id];
    mp_sched_schedule(handler, MP_OBJ_FROM_PTR(&machine_pin_obj[self->id]));
    mp_hal_wake_main_task_from_isr();
}

/******************************************************************************/
// Pin IRQ object

STATIC const mp_obj_type_t machine_pin_irq_type;

// pin.irq(handler=None, trigger=IRQ_FALLING|IRQ_RISING)
STATIC mp_obj_t machine_pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = AM_HAL_GPIO_PIN_INTDIR_BOTH} },
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    machine_pin_irq_obj_t *irq_obj = NULL;
    mp_obj_t handler = mp_const_none;
    uint32_t trigger = 0;

    if (n_args > 1 || kw_args->used != 0) {
        // configure irq
        handler = args[ARG_handler].u_obj;
        trigger = args[ARG_trigger].u_int;
    }

    if (handler == mp_const_none) {
        handler = MP_OBJ_NULL;
        trigger = AM_HAL_GPIO_PIN_INTDIR_BOTH;
    }

    // setup ISR object
    irq_obj = &machine_pin_irq_object[self->id];
    irq_obj->base.type = &machine_pin_irq_type;
    irq_obj->id = self->id;
    irq_obj->mode = trigger;
    

    uint8_t pad = ap3_pin2pad[self->id];
    AM_HAL_GPIO_MASKCREATE(IntMask);
    AM_HAL_GPIO_MASKBIT(pIntMask, pad);
    am_hal_gpio_interrupt_disable(pIntMask);
    am_hal_gpio_interrupt_clear(pIntMask);
    if (handler != MP_OBJ_NULL) {
        am_hal_gpio_pincfg_t new_cfg;
        ap3_get_pincfg(pad, &new_cfg);
  
        MP_STATE_PORT(machine_pin_irq_handler)[self->id] = handler;
        new_cfg.eIntDir = trigger;
        am_hal_gpio_pinconfig(pad, new_cfg);
        am_hal_gpio_interrupt_register_adv(pad, machine_pin_isr_handler, (void *)irq_obj);
        am_hal_gpio_interrupt_enable(pIntMask);
        // Check if GPIO isr vector enabled
        if (!NVIC_GetActive(GPIO_IRQn)) {
            NVIC_EnableIRQ(GPIO_IRQn);
        }
    }

    // return the irq object
    return MP_OBJ_FROM_PTR(irq_obj);
}

STATIC mp_obj_t machine_pin_irq_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = self_in;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    machine_pin_isr_handler(self);
    return mp_const_none;
}

STATIC mp_obj_t machine_pin_irq_trigger(size_t n_args, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = args[0];
    uint32_t orig_trig = self->mode;
    // Set new trigger request if given
    if (n_args == 2) {
        am_hal_gpio_pincfg_t c_cfg;
        uint8_t pad = ap3_pin2pad[self->id];
        ap3_get_pincfg(pad, &c_cfg);
        self->mode = mp_obj_get_int(args[1]);
        c_cfg.eIntDir = self->mode;
        am_hal_gpio_pinconfig(pad, c_cfg);
    }

    // return original trigger value
    return MP_OBJ_NEW_SMALL_INT(orig_trig);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_irq_trigger_obj, 1, 2, machine_pin_irq_trigger);

STATIC const mp_rom_map_elem_t machine_pin_irq_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_trigger), MP_ROM_PTR(&machine_pin_irq_trigger_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_irq_locals_dict, machine_pin_irq_locals_dict_table);

STATIC const mp_obj_type_t machine_pin_irq_type = {
    { &mp_type_type },
    .name = MP_QSTR_IRQ,
    .call = machine_pin_irq_call,
    .locals_dict = (mp_obj_dict_t *)&machine_pin_irq_locals_dict,
};
