/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
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
#include "am_hal_iom.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/machine_i2c.h"

#include "FreeRTOS.h"
#include "semphr.h"

// I2C pads
#define I2C_0_DEFAULT_SCL (39)
#define I2C_0_DEFAULT_SDA (40)
#define I2C_1_DEFAULT_SCL (42)
#define I2C_1_DEFAULT_SDA (43)

#define I2C_DEFAULT_TIMEOUT_US (10000) // 10ms

typedef struct _machine_hw_i2c_obj_t {
    mp_obj_base_t base;
    void *iom;          // IOM handle
    uint32_t freq;      // selected speed
    SemaphoreHandle_t semaphore;
    // IOM and pins
    uint8_t port : 8;
    uint8_t scl : 8;
    uint8_t sda : 8;
    uint8_t has_lock : 8;
} machine_hw_i2c_obj_t;

STATIC machine_hw_i2c_obj_t machine_hw_i2c_obj[AP3_I2C_INTERFACES];

const am_hal_gpio_pincfg_t AP3_I2C_GPIO_DEFAULT =
{
    .uFuncSel       = 4,
    .ePullup        = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN
};

STATIC void machine_hw_i2c_init(machine_hw_i2c_obj_t *self, uint32_t freq, uint32_t timeout_us) {
    // Clear previous config if exists
    if (self->iom) {
        am_hal_iom_disable(self->iom);
        am_hal_iom_power_ctrl(self->iom, AM_HAL_SYSCTRL_DEEPSLEEP, false);
        am_hal_iom_uninitialize(self->iom);
        self->iom = NULL;
    }

    am_hal_iom_config_t iom_cfg = {0};
    iom_cfg.eInterfaceMode = AM_HAL_IOM_I2C_MODE;
    iom_cfg.ui32ClockFreq = freq;
    
    am_hal_iom_initialize(self->port, &self->iom);
    if (self->iom == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid I2C port"));
    }
    // Save for Print
    self->freq = freq;
    
    am_hal_iom_power_ctrl(self->iom, AM_HAL_SYSCTRL_WAKE, false);
    
    if (am_hal_iom_configure(self->iom, &iom_cfg)  != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("I2C config error"));
    }
    am_hal_iom_enable(self->iom);

    // Configure the IOM pins
    am_hal_gpio_pincfg_t pincfg = AP3_I2C_GPIO_DEFAULT;
    pincfg.uIOMnum = self->port;
     
    // configure the pin as requested
    if (am_hal_gpio_pinconfig(self->scl, pincfg) != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("SCL config error"));
    }
  
     // configure the pin as requested
    if (am_hal_gpio_pinconfig(self->sda, pincfg) != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("SDA config error"));
    } 
}

int machine_hw_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    am_hal_iom_transfer_t iomTransfer = {0};

    iomTransfer.uPeerInfo.ui32I2CDevAddr = addr;
    iomTransfer.bContinue = true;
    iomTransfer.ui8Priority = 1;

    int data_len = 0;
    for (; n--; ++bufs) {
        uint32_t retval;
        // Check for stop on last buffer only
        if (n == 0) {
            iomTransfer.bContinue = (flags & MP_MACHINE_I2C_FLAG_STOP) ? false : true;
        }

        if (flags & MP_MACHINE_I2C_FLAG_READ) {
            iomTransfer.eDirection = AM_HAL_IOM_RX;
            iomTransfer.pui32RxBuffer = (uint32_t *)bufs->buf;
        } else {
            iomTransfer.eDirection = AM_HAL_IOM_TX;
            iomTransfer.pui32TxBuffer = (uint32_t *)bufs->buf;
        }


        iomTransfer.ui32NumBytes = bufs->len;
        retval = am_hal_iom_blocking_transfer(self->iom, &iomTransfer);
        // Check errors
        if (retval == AM_HAL_STATUS_TIMEOUT) {
            return -MP_ETIMEDOUT;
        } else if (retval != AM_HAL_STATUS_SUCCESS) {
            return -MP_EIO;
        }
        // Accumulate xfered amount
        data_len += bufs->len;
    }

    return data_len;
}

/******************************************************************************/
// MicroPython bindings for machine API

STATIC void machine_hw_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "I2C(%u, scl=%u, sda=%u, freq=%u)",
        self->port, self->scl, self->sda, self->freq);
}

mp_obj_t machine_hw_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Parse args
    enum { ARG_id, ARG_scl, ARG_sda, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = AM_HAL_IOM_100KHZ} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = I2C_DEFAULT_TIMEOUT_US} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus
    mp_int_t i2c_id = mp_obj_get_int(args[ARG_id].u_obj);
    if (!(0 <= i2c_id && i2c_id < AP3_I2C_INTERFACES)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }

    // Get static peripheral object
    machine_hw_i2c_obj_t *self = (machine_hw_i2c_obj_t *)&machine_hw_i2c_obj[i2c_id];
    if (self->base.type == NULL) {
        // Created for the first time, set default pins
        self->base.type = &machine_hw_i2c_type;
        if (i2c_id == 0) {
            self->port = AP3_I2C0_IOM;
            self->scl = I2C_0_DEFAULT_SCL;
            self->sda = I2C_0_DEFAULT_SDA;
        } else {
            self->port = AP3_I2C1_IOM;
            self->scl = I2C_1_DEFAULT_SCL;
            self->sda = I2C_1_DEFAULT_SDA;
        }
    }

    // Set SCL/SDA pins if given
    if (args[ARG_scl].u_obj != MP_OBJ_NULL) {
        self->scl = mp_hal_get_pin_obj(args[ARG_scl].u_obj);
    }
    if (args[ARG_sda].u_obj != MP_OBJ_NULL) {
        self->sda = mp_hal_get_pin_obj(args[ARG_sda].u_obj);
    }

    // Create binary semaphore for I2C bus serializaiton
    if (!self->semaphore) {
        self->semaphore = xSemaphoreCreateBinary();
        if (!self->semaphore) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Unable to create lock"));
        }
    }

    // Release semaphore initially
    self->has_lock = false;
    xSemaphoreGive(self->semaphore);

    // Initialise the I2C peripheral
    machine_hw_i2c_init(self, args[ARG_freq].u_int, args[ARG_timeout].u_int);

    return MP_OBJ_FROM_PTR(self);
}

STATIC bool i2c_try_lock(machine_hw_i2c_obj_t *self) {
    if (self->has_lock) {
        return false;
    }
    self->has_lock = (xSemaphoreTake(self->semaphore, 0) == pdTRUE);
    return self->has_lock;
}

STATIC void i2c_unlock(machine_hw_i2c_obj_t *self) {
    xSemaphoreGive(self->semaphore);
    self->has_lock = false;
}

STATIC mp_obj_t i2c_obj_try_lock(mp_obj_t self_in) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(i2c_try_lock(self));
}
MP_DEFINE_CONST_FUN_OBJ_1(i2c_try_lock_obj, i2c_obj_try_lock);

STATIC mp_obj_t i2c_obj_unlock(mp_obj_t self_in) {
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    i2c_unlock(self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(i2c_unlock_obj, i2c_obj_unlock);

STATIC int machine_hw_i2c_readfrom(mp_obj_base_t *self, uint16_t addr, uint8_t *dest, size_t len, bool stop) {
    mp_machine_i2c_buf_t buf = {.len = len, .buf = dest};
    unsigned int flags = MP_MACHINE_I2C_FLAG_READ | (stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
    return machine_hw_i2c_transfer(self, addr, 1, &buf, flags);
}

STATIC int machine_hw_i2c_writeto(mp_obj_base_t *self, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    mp_machine_i2c_buf_t buf = {.len = len, .buf = (uint8_t *)src};
    unsigned int flags = stop ? MP_MACHINE_I2C_FLAG_STOP : 0;
    return machine_hw_i2c_transfer(self, addr, 1, &buf, flags);
}

STATIC mp_obj_t machine_i2c_readfrom(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[2]));
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    int ret = machine_hw_i2c_readfrom(self, addr, (uint8_t *)vstr.buf, vstr.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_obj, 3, 4, machine_i2c_readfrom);

STATIC mp_obj_t machine_i2c_readfrom_into(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    int ret = machine_hw_i2c_readfrom(self, addr, bufinfo.buf, bufinfo.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_into_obj, 3, 4, machine_i2c_readfrom_into);

STATIC mp_obj_t machine_i2c_writeto(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    int ret = machine_hw_i2c_writeto(self, addr, bufinfo.buf, bufinfo.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    // return number of acks received
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_writeto_obj, 3, 4, machine_i2c_writeto);

STATIC mp_obj_t machine_i2c_scan(mp_obj_t self_in) {
    mp_obj_base_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    uint8_t bfr[4];
    // 7-bit addresses 0b0000xxx and 0b1111xxx are reserved
    for (int addr = 0x08; addr < 0x78; ++addr) {
        int ret = machine_hw_i2c_readfrom(self, addr, bfr, 1, true);
        if (ret == 0) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
    }
    return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_scan_obj, machine_i2c_scan);

STATIC const mp_machine_i2c_p_t machine_hw_i2c_p = {
    .transfer = machine_hw_i2c_transfer,
};

STATIC const mp_rom_map_elem_t machine_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&machine_i2c_scan_obj) },

    { MP_ROM_QSTR(MP_QSTR_try_lock), MP_ROM_PTR(&i2c_try_lock_obj) },
    { MP_ROM_QSTR(MP_QSTR_unlock), MP_ROM_PTR(&i2c_unlock_obj) },

    { MP_ROM_QSTR(MP_QSTR_readfrom), MP_ROM_PTR(&machine_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into), MP_ROM_PTR(&machine_i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto), MP_ROM_PTR(&machine_i2c_writeto_obj) },
};

STATIC MP_DEFINE_CONST_DICT(machine_i2c_locals_dict, machine_i2c_locals_dict_table);

const mp_obj_type_t machine_hw_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = machine_hw_i2c_print,
    .make_new = machine_hw_i2c_make_new,
    .protocol = &machine_hw_i2c_p,
    .locals_dict = (mp_obj_dict_t *)&machine_i2c_locals_dict,
};
