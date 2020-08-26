//*****************************************************************************
//
//! @file mp_rtos.c
//!
//! @brief Essential functions to make the RTOS run correctly.
//!
//! These functions are required by the RTOS for ticking, sleeping, and basic
//! error checking.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 2.4.2 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include "mp_rtos.h"

//*****************************************************************************
//
// Task handle for the main Micropython task.
//
//*****************************************************************************
TaskHandle_t xMicroPyTask;


//*****************************************************************************
//
// Sleep function called from FreeRTOS IDLE task.
// Do necessary application specific Power down operations here
// Return 0 if this function also incorporates the WFI, else return value same
// as idleTime
//
//*****************************************************************************
uint32_t am_freertos_sleep(uint32_t idleTime)
{
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    return 0;
}

//*****************************************************************************
//
// Recovery function called from FreeRTOS IDLE task, after waking up from Sleep
// Do necessary 'wakeup' operations here, e.g. to power up/enable peripherals etc.
//
//*****************************************************************************
void am_freertos_wakeup(uint32_t idleTime)
{
    return;
}


//*****************************************************************************
//
// FreeRTOS debugging functions.
//
//*****************************************************************************
void
vApplicationMallocFailedHook(void)
{
    //
    // Called if a call to pvPortMalloc() fails because there is insufficient
    // free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    // internally by FreeRTOS API functions that create tasks, queues, software
    // timers, and semaphores.  The size of the FreeRTOS heap is set by the
    // linker script by the _heap_size / _heap_start declarations.
    //
    while (1)
   {
        __asm("BKPT #0\n") ; // Break into the debugger
   }
}

void
vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void) pcTaskName;
    (void) pxTask;

    //
    // Run time stack overflow checking is performed if
    // configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    // function is called if a stack overflow is detected.
    //
    while (1)
    {
        __asm("BKPT #0\n") ; // Break into the debugger
    }
}

// Micropython task entry
extern void mp_task(void *pvParameter);

// External FreeRTOS heap definitions (see heap_5.c)
extern uint32_t _heap_start;
extern uint32_t _heap_size;

HeapRegion_t mpy_heap[2] = {0};

//*****************************************************************************
//
// Initializes all tasks
//
//*****************************************************************************
void run_tasks(void)
{
    //
    // Set some interrupt priorities before we create tasks or start the scheduler.
    //
    // Note: Timer priority is handled by the FreeRTOS kernel, so we won't
    // touch it here.
    //

    //
    // Print a debug message.
    //
    am_util_debug_printf("Running tasks...\r\n");

    //
    // Create a heap from remaining memory
    //
    mpy_heap[0].pucStartAddress = (uint8_t *)&_heap_start;
    mpy_heap[0].xSizeInBytes = (size_t)&_heap_size;

    vPortDefineHeapRegions(mpy_heap);

    // ****TODO: BLE support
    // RadioTaskSetup();
    // xTaskCreate(RadioTask, "RadioTask", 512, 0, 3, &radio_task_handle);

    //
    // Create the Micropython task.
    //
    xTaskCreate(mp_task, "mp_task", MP_TASK_STACK_SIZE / sizeof(StackType_t),
                NULL, MP_TASK_PRIORITY, &xMicroPyTask);

    //
    // Start the scheduler.
    //
    vTaskStartScheduler();
}

