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

#include <stdio.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "mphalport.h"
#include "mp_rtos.h"

void *g_phUART;

static const am_hal_uart_config_t sUartConfig =
{
    // Standard UART settings: 115200-8-N-1
    .ui32BaudRate = 115200,
    .ui32DataBits = AM_HAL_UART_DATA_BITS_8,
    .ui32Parity = AM_HAL_UART_PARITY_NONE,
    .ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
    .ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

    // Set TX and RX FIFOs to interrupt at half-full.
    .ui32FifoLevels = (AM_HAL_UART_TX_FIFO_1_2 |
                       AM_HAL_UART_RX_FIFO_1_2),

    // Buffers
    .pui8TxBuffer = NULL,
    .ui32TxBufferSize = 0,
    .pui8RxBuffer = NULL,
    .ui32RxBufferSize = 0,
};


void uart_init(void) {
    //
    // Initialize the printf interface for UART output.
    //
    am_hal_uart_initialize(0, &g_phUART);
    am_hal_uart_power_control(g_phUART, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_uart_configure(g_phUART, &sUartConfig);

    // Disable that pesky FIFO
    UARTn(0)->LCRH_b.FEN = 0;

    //
    // Enable the UART pins.
    //
    am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);

    //
    // Enable interrupts.
    //
    NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + AM_BSP_UART_PRINT_INST));
    am_hal_uart_interrupt_enable(g_phUART, (AM_HAL_UART_INT_RX));

    return;
}

void am_uart_isr(void) {
    uint32_t ui32Status;

    am_hal_uart_interrupt_status_get(g_phUART, &ui32Status, true);
    am_hal_uart_interrupt_clear(g_phUART, ui32Status);
    //am_hal_uart_interrupt_service(g_phUART, ui32Status, NULL);

    uint8_t c = 0;
    if (ui32Status & AM_HAL_UART_INT_RX)
    {
        uint32_t ui32BytesRead = 0;
        am_hal_uart_transfer_t sRead = {
            .ui32Direction = AM_HAL_UART_READ,
            .pui8Data = &c,
            .ui32NumBytes = 1,
            .ui32TimeoutMs = 0,
            .pui32BytesTransferred = &ui32BytesRead,
        };
        am_hal_uart_transfer(g_phUART, &sRead);

        if (ui32BytesRead > 0)
        {
            if (c == mp_interrupt_char) {
                mp_keyboard_interrupt();
            } else {
               ringbuf_put(&stdin_ringbuf, c);
            }
	}
    }
}
