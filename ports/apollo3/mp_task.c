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

#include "mp_rtos.h"

#include "py/stackctrl.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/persistentcode.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "lib/mp-readline/readline.h"
#include "lib/utils/pyexec.h"

#include "am_hal_sysctrl.h"
//#include "modmachine.h"
#include "uart.h"

#if MICROPY_PY_THREAD
#include "mpthreadport.h"
#endif

void mp_task(void *pvParameter) {
    volatile uint32_t sp = (uint32_t)get_sp();
#if MICROPY_PY_THREAD
    mp_thread_init(pxTaskGetStackStart(NULL), MP_TASK_STACK_SIZE / sizeof(uintptr_t));
#endif
    uart_init();

    // Allocate the uPy heap using malloc and get the largest available region
    // ****TODO: Don't use all of heap memory
    size_t mp_task_heap_size = xPortGetFreeHeapSize() - (16 * 1024);
    void *mp_task_heap = pvPortMalloc(mp_task_heap_size - portBYTE_ALIGNMENT);

soft_reset:
    // initialise the stack pointer for the main thread
    mp_stack_set_top((void *)sp);
    mp_stack_set_limit(MP_TASK_STACK_SIZE - 1024);
    gc_init(mp_task_heap, mp_task_heap + mp_task_heap_size);
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
    mp_obj_list_init(mp_sys_argv, 0);
    readline_init0();

    // ****TODO: initialise peripherals
    // machine_pins_init();

#if MICROPY_MODULE_FROZEN
    // run boot-up scripts
    pyexec_frozen_module("_boot.py");
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
        pyexec_file_if_exists("main.py");
    }
#endif
    for (;;) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

    #if MICROPY_BLUETOOTH_NIMBLE
    mp_bluetooth_deinit();
    #endif

    // ****TODO: timers, etc.
    // machine_timer_deinit_all();

    #if MICROPY_PY_THREAD
    mp_thread_deinit();
    #endif

    gc_sweep_all();

    mp_hal_stdout_tx_str("MPY: soft reboot\r\n");

    // ****TODO: deinitialise peripherals
    //machine_pins_deinit();

    mp_deinit();
    am_hal_uart_tx_flush(g_phUART);
    goto soft_reset;
}

void NORETURN __fatal_error(const char *msg) {
    am_hal_uart_tx_flush(g_phUART);
    am_hal_sysctrl_aircr_reset();
    while (1) {
	// Flash some LEDs
        am_devices_led_on(am_bsp_psLEDs, 0);
    }
}

void nlr_jump_fail(void *val) {
    am_util_stdio_printf("FATAL: uncaught exception %p\n", val);
    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)val);
    __fatal_error("");
}

void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    am_util_stdio_printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}

#ifdef SEGGER_ASSERT
void MP_WEAK __assert(const char *expr, const char *file, int line) {
    __assert_func(file, line, NULL, expr);
}
#endif
