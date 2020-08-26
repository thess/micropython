/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
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
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "am_mcu_apollo.h"

#include "py/obj.h"
#include "py/objstr.h"
#include "py/stream.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "py/gc.h"
#include "extmod/misc.h"
#include "lib/utils/pyexec.h"
#include "lib/utils/gchelper.h"

#include "mphalport.h"
#include "uart.h"

TaskHandle_t mp_main_task_handle;

static uint8_t stdin_ringbuf_array[256];
ringbuf_t stdin_ringbuf = {stdin_ringbuf_array, sizeof(stdin_ringbuf_array)};

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;

    if ((poll_flags & MP_STREAM_POLL_RD) && 
        (stdin_ringbuf.iget != stdin_ringbuf.iput)) {

        ret |= MP_STREAM_POLL_RD;
    }
    return ret;
}

int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int c = ringbuf_get(&stdin_ringbuf);
        if (c != -1) {
            return c;
        }
        MICROPY_EVENT_POLL_HOOK
        ulTaskNotifyTake(pdFALSE, 1);
    }
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    uint32_t ui32BytesWritten = 0;

#if MICROPY_PY_THREAD
    // Only release the GIL if many characters are being sent
    bool release_gil = len > 20;
    if (release_gil) {
        MP_THREAD_GIL_EXIT();
    }
#endif
    //
    // Print the string via the UART.
    //
    const am_hal_uart_transfer_t sUartWrite =
    {
        .ui32Direction = AM_HAL_UART_WRITE,
        .pui8Data = (uint8_t *) str,
        .ui32NumBytes = len,
        .ui32TimeoutMs = AM_HAL_UART_WAIT_FOREVER,
        .pui32BytesTransferred = &ui32BytesWritten,
    };

    am_hal_uart_transfer(g_phUART, &sUartWrite);

#if MICROPY_PY_THREAD
    if (release_gil) {
        MP_THREAD_GIL_ENTER();
    }
#endif
    mp_uos_dupterm_tx_strn(str, len);
}

// Efficiently convert "\n" to "\r\n"
// AM textmode only works for int printf()
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    const char *last = str;
    while (len--) {
        if (*str == '\n') {
            if (str > last) {
                mp_hal_stdout_tx_strn(last, str - last);
            }
            mp_hal_stdout_tx_strn("\r\n", 2);
            ++str;
            last = str;
        } else {
            ++str;
        }
    }
    if (str > last) {
        mp_hal_stdout_tx_strn(last, str - last);
    }
}

void mp_hal_delay_ms(uint32_t ms) {
    uint64_t us = ms * 1000;
    uint64_t dt;
    uint64_t t0 = mp_hal_ticks_us();
    for (;;) {
        uint64_t t1 = mp_hal_ticks_us();
        dt = t1 - t0;
        if (dt + portTICK_PERIOD_MS * 1000 >= us) {
            // doing a vTaskDelay would take us beyond requested delay time
            break;
        }
        MICROPY_EVENT_POLL_HOOK
        ulTaskNotifyTake(pdFALSE, 1);
    }
    if (dt < us) {
        // do the remaining delay accurately
        mp_hal_delay_us(us - dt);
    }
}

void mp_hal_delay_us(uint32_t us) {
    // these constants are tested for a 240MHz clock
    const uint32_t this_overhead = 5;
    const uint32_t pend_overhead = 150;

    // return if requested delay is less than calling overhead
    if (us < this_overhead) {
        return;
    }
    us -= this_overhead;

    uint64_t t0 = mp_hal_ticks_us();
    for (;;) {
        uint64_t dt = mp_hal_ticks_us() - t0;
        if (dt >= us) {
            return;
        }
        if (dt + pend_overhead < us) {
            // we have enough time to service pending events
            // (don't use MICROPY_EVENT_POLL_HOOK because it also yields)
            mp_handle_pending(true);
            __WFI();
        }
    }
}


// Wake up the main task if it is sleeping
void mp_hal_wake_main_task_from_isr(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(mp_main_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Canonical GC for wno
void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // trace the stack and the registers
    gc_helper_collect_regs_and_stack();

    // trace root pointers from any threads
#if MICROPY_PY_THREAD
    mp_thread_gc_others();
#endif

    // end the GC
    gc_collect_end();
}
