/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
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

#include <stdio.h>

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "am_hal_flash.h"

// End of resident flash data
extern uint32_t _etext;
extern uint32_t _flash_size;

STATIC mp_obj_t artemis_flash_read(mp_obj_t offset_in, mp_obj_t len_or_buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);

    mp_int_t len;
    byte *buf;
    bool alloc_buf = mp_obj_is_int(len_or_buf_in);

    if (alloc_buf) {
        len = mp_obj_get_int(len_or_buf_in);
        buf = m_new(byte, len);
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(len_or_buf_in, &bufinfo, MP_BUFFER_WRITE);
        len = bufinfo.len;
        buf = bufinfo.buf;
    }

    if ((offset - len) >= (uint32_t)&_flash_size) {
        mp_raise_ValueError(MP_ERROR_TEXT("read offset out-of-range"));
    }
    // We know that allocation will be 4-byte aligned for sure
    memcpy(buf, (byte *)offset, len);
    if (alloc_buf) {
        return mp_obj_new_bytes(buf, len);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(artemis_flash_read_obj, artemis_flash_read);

STATIC mp_obj_t artemis_flash_write(mp_obj_t offset_in, const mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    if (bufinfo.len & 0x3) {
        mp_raise_ValueError(MP_ERROR_TEXT("buflen not multiple of 4"));
    }
    if (offset <= (uint32_t)&_etext) {
        mp_raise_ValueError(MP_ERROR_TEXT("write offset out-of-range"));
    }

    // Must disable ints during flash writes
    vPortEnterCritical();
    int res = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY, bufinfo.buf,
                                        (uint32_t *)offset, bufinfo.len >> 2);
    vPortExitCritical();
    if (!res) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(artemis_flash_write_obj, artemis_flash_write);

STATIC mp_obj_t artemis_flash_erase(mp_obj_t sector_in) {
    mp_int_t sector = mp_obj_get_int(sector_in);
    if (sector <= (uint32_t)&_etext / AM_HAL_FLASH_PAGE_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("erase sector out-of-range"));
    }

   // Must disable ints during flash writes
    vPortEnterCritical();
    int res = am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                                  (sector >> 6) & (AM_HAL_FLASH_NUM_INSTANCES - 1),
                                  sector & 0x3F);
    vPortExitCritical();
    if (!res) {
        return mp_const_none;
    }
    mp_raise_OSError(MP_EIO);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(artemis_flash_erase_obj, artemis_flash_erase);

STATIC mp_obj_t artemis_flash_size(void) {
    return mp_obj_new_int_from_uint(AM_HAL_FLASH_TOTAL_SIZE);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(artemis_flash_size_obj, artemis_flash_size);

STATIC mp_obj_t artemis_flash_user_start(void) {
    return mp_obj_new_int_from_uint((uint32_t)&_etext);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(artemis_flash_user_start_obj, artemis_flash_user_start);

STATIC const mp_rom_map_elem_t artemis_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_artemis) },

    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&artemis_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&artemis_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&artemis_flash_erase_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_PTR(&artemis_flash_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_start), MP_ROM_PTR(&artemis_flash_user_start_obj) },
};

STATIC MP_DEFINE_CONST_DICT(artemis_module_globals, artemis_module_globals_table);

const mp_obj_module_t artemis_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&artemis_module_globals,
};
