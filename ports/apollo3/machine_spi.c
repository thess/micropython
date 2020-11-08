/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
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
#include "py/stream.h"
#include "py/mphal.h"
#include "extmod/machine_spi.h"

#include "FreeRTOS.h"
#include "semphr.h"

typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    void *iom;          // IOM handle
    SemaphoreHandle_t semaphore;
    uint32_t freq;      // selected speed
    uint8_t port;
    uint8_t has_lock;
    uint8_t mode;       // [Polarity]:[Phase]
    uint8_t bits;
    uint8_t firstbit;
    mp_hal_pin_obj_t sck;
    mp_hal_pin_obj_t mosi;
    mp_hal_pin_obj_t miso;
} machine_hw_spi_obj_t;

STATIC machine_hw_spi_obj_t machine_hw_spi_obj[AP3_SPI_INTERFACES];

const am_hal_gpio_pincfg_t AP3_SPI_GPIO_DEFAULT =
{
    .uFuncSel       = 5,
    .ePullup        = AM_HAL_GPIO_PIN_PULLUP_1_5K,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_OPENDRAIN
};

STATIC void machine_hw_spi_deinit_internal(machine_hw_spi_obj_t *self) {
    // reset IOM
    am_hal_iom_disable(self->iom);
    am_hal_iom_power_ctrl(self->iom, AM_HAL_SYSCTRL_DEEPSLEEP, false);
    am_hal_iom_uninitialize(self->iom);
    self->iom = NULL;

    int8_t pads[3] = {self->miso, self->mosi, self->sck};

    for (int8_t p = 0; p < 3; p++) {
        if (pads[p] != -1) {
            // Pin reset
            am_hal_gpio_pinconfig(pads[p], g_AM_HAL_GPIO_DISABLE);
        }
    }
}

STATIC void machine_hw_spi_init_internal(
    machine_hw_spi_obj_t *self,
    int32_t freq,
    int8_t mode,
    int8_t bits,
    int8_t firstbit,
    mp_hal_pin_obj_t sck,
    mp_hal_pin_obj_t mosi,
    mp_hal_pin_obj_t miso) {

    // Clear previous config if exists
    if (self->iom) {
        machine_hw_spi_deinit_internal(self);
    }
    // Override defaults if requested
    if (freq != -1 && freq != self->freq) {
        if (freq > AM_HAL_IOM_MAX_FREQ)
            freq = AM_HAL_IOM_MAX_FREQ;
        self->freq = freq;
    }

    if (mode != -1 && mode != self->mode) {
        self->mode = mode;
    }

    if (bits != -1 && bits != self->bits) {
        self->bits = bits;
    }

    if (firstbit != -1 && firstbit != self->firstbit) {
        self->firstbit = firstbit;
    }

    if (sck != -2 && sck != self->sck) {
        self->sck = sck;
    }

    if (mosi != -2 && mosi != self->mosi) {
        self->mosi = mosi;
    }

    if (miso != -2 && miso != self->miso) {
        self->miso = miso;
    }
    
    am_hal_iom_config_t iom_cfg = {0};
    iom_cfg.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
    iom_cfg.eSpiMode = self->mode;
    iom_cfg.ui32ClockFreq = self->freq;
    
    am_hal_iom_initialize(self->port, &self->iom);
    if (self->iom == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid SPI port"));
    }

    am_hal_iom_power_ctrl(self->iom, AM_HAL_SYSCTRL_WAKE, false);
    
    if (am_hal_iom_configure(self->iom, &iom_cfg)  != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("SPI config error"));
    }
    am_hal_iom_enable(self->iom);

    // Set up pins
    am_hal_gpio_pincfg_t pincfg = AP3_SPI_GPIO_DEFAULT;
    pincfg.uIOMnum = self->port;
    // clock
    if (am_hal_gpio_pinconfig(self->sck, pincfg) != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("SCK config error"));
    }
    // output
    if (am_hal_gpio_pinconfig(self->miso, pincfg) != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("MISO config error"));
    }
    // input
    if (am_hal_gpio_pinconfig(self->mosi, pincfg) != AM_HAL_STATUS_SUCCESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("MOSI config error"));
    }
}

STATIC void machine_hw_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;
    machine_hw_spi_deinit_internal(self);
}

STATIC void machine_hw_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);

    am_hal_iom_transfer_t iomTransfer = {0};

    iomTransfer.ui32NumBytes = len;
    iomTransfer.pui32TxBuffer = (uint32_t *)dest;
    iomTransfer.pui32RxBuffer = (uint32_t *)src;
    iomTransfer.ui8Priority = 1;

    // Determine direction
    if ((dest != NULL) && (src != NULL))
    {
        iomTransfer.eDirection = AM_HAL_IOM_FULLDUPLEX;
    }
    else if (dest != NULL)
    {
        iomTransfer.eDirection = AM_HAL_IOM_TX;
    }
    else if (src != NULL)
    {
        iomTransfer.eDirection = AM_HAL_IOM_RX;
    }

    uint32_t retval = 0;
    if (iomTransfer.eDirection == AM_HAL_IOM_FULLDUPLEX)
    {
        retval = am_hal_iom_spi_blocking_fullduplex(self->iom, &iomTransfer);
    }
    else
    {
        retval = am_hal_iom_blocking_transfer(self->iom, &iomTransfer);
    }

    // Check errors
    if (retval == AM_HAL_STATUS_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (retval != AM_HAL_STATUS_SUCCESS) {
        mp_raise_OSError(MP_EIO);
    }
    
    return;
}

/******************************************************************************/
// MicroPython bindings for hw_spi

STATIC void machine_hw_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(%u, baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%u, sck=%d, mosi=%d, miso=%d)",
        self->port, self->freq, (self->mode & 0x02) ? 1 : 0,
        (self->mode & 0x01), self->bits, self->firstbit,
        self->sck, self->mosi, self->miso);
}

STATIC void machine_hw_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;

    enum { ARG_freq, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    int8_t sck, mosi, miso;

    if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
        sck = -2;
    } else if (args[ARG_sck].u_obj == mp_const_none) {
        sck = -1;
    } else {
        sck = mp_hal_get_pin_obj(args[ARG_sck].u_obj);
    }

    if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
        miso = -2;
    } else if (args[ARG_miso].u_obj == mp_const_none) {
        miso = -1;
    } else {
        miso = mp_hal_get_pin_obj(args[ARG_miso].u_obj);
    }

    if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
        mosi = -2;
    } else if (args[ARG_mosi].u_obj == mp_const_none) {
        mosi = -1;
    } else {
        mosi = mp_hal_get_pin_obj(args[ARG_mosi].u_obj);
    }

    uint8_t spimode = (args[ARG_polarity].u_int) ? 2 : 0 | (args[ARG_phase].u_int) ? 1 : 0;
    machine_hw_spi_init_internal(self, args[ARG_freq].u_int, spimode,
        args[ARG_bits].u_int, args[ARG_firstbit].u_int, sck, mosi, miso);
}

mp_obj_t machine_hw_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    MP_MACHINE_SPI_CHECK_FOR_LEGACY_SOFTSPI_CONSTRUCTION(n_args, n_kw, all_args);

    enum { ARG_id, ARG_freq, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 500000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_PY_MACHINE_SPI_MSB} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get SPI bus
    mp_int_t spi_id = mp_obj_get_int(args[ARG_id].u_obj);
    if (!(0 <= spi_id && spi_id < AP3_SPI_INTERFACES)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), spi_id);
    }

    // Get static peripheral object
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)&machine_hw_spi_obj[spi_id];
    if (self->base.type == NULL) {
        // Created for the first time, set default pins
        self->base.type = &machine_hw_spi_type;
        self->port = AP3_SPI_IOM0;
        self->sck = SPI_0_DEFAULT_SCK;
        self->mosi = SPI_0_DEFAULT_MOSI;
        self->miso = SPI_0_DEFAULT_MISO;
    }
    
    // Pin config (if given)
    if (args[ARG_sck].u_obj != MP_OBJ_NULL) {
        self->sck = mp_hal_get_pin_obj(args[ARG_sck].u_obj);
    }
    if (args[ARG_mosi].u_obj != MP_OBJ_NULL) {
        self->mosi = mp_hal_get_pin_obj(args[ARG_mosi].u_obj);
    }
    if (args[ARG_miso].u_obj != MP_OBJ_NULL) {
        self->miso = mp_hal_get_pin_obj(args[ARG_miso].u_obj);
    }
    
    // Create binary semaphore for SPI bus serializaiton
    if (!self->semaphore) {
        self->semaphore = xSemaphoreCreateBinary();
        if (!self->semaphore) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("Unable to create lock"));
        }
    }

    // Release semaphore initially
    self->has_lock = false;
    xSemaphoreGive(self->semaphore);

    uint8_t spimode = (args[ARG_polarity].u_int) ? 2 : 0 | (args[ARG_phase].u_int) ? 1 : 0;
    machine_hw_spi_init_internal(
        self,
        args[ARG_freq].u_int,
        spimode,
        args[ARG_bits].u_int,
        args[ARG_firstbit].u_int,
        args[ARG_sck].u_obj == MP_OBJ_NULL ? -1 : machine_pin_get_id(args[ARG_sck].u_obj),
        args[ARG_mosi].u_obj == MP_OBJ_NULL ? -1 : machine_pin_get_id(args[ARG_mosi].u_obj),
        args[ARG_miso].u_obj == MP_OBJ_NULL ? -1 : machine_pin_get_id(args[ARG_miso].u_obj));

    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_machine_spi_p_t machine_hw_spi_p = {
    .init = machine_hw_spi_init,
    .deinit = machine_hw_spi_deinit,
    .transfer = machine_hw_spi_transfer,
};

const mp_obj_type_t machine_hw_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = machine_hw_spi_print,
    .make_new = machine_hw_spi_make_new,
    .protocol = &machine_hw_spi_p,
    .locals_dict = (mp_obj_dict_t *)&mp_machine_spi_locals_dict,
};
