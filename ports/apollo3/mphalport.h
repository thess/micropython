// HAL definitions for Apollo3 serial and clock

/** @brief Read current stack pointer address
 *
 */

#ifndef MICROPY_INCLUDED_APOLLO3_MPHALPORT_H
#define MICROPY_INCLUDED_APOLLO3_MPHALPORT_H

#include "FreeRTOS.h"
#include "task.h"

#include "lib/utils/interrupt_char.h"
#include "py/ringbuf.h"

void gc_collect(void);

__attribute__((always_inline)) static inline void *get_sp()
{
    void *sp;
    __asm volatile ("mov %0, sp;" : "=r" (sp));
    return sp;
}

__attribute__((always_inline)) static inline uint32_t mp_hal_ticks_cpu(void)
{
    return am_hal_stimer_counter_get();
}

__attribute__((always_inline)) static inline uint32_t mp_hal_ticks_us(void)
{
    return ((uint64_t)am_hal_stimer_counter_get() * 1000000ULL) / configSTIMER_CLOCK_HZ ;
}

__attribute__((always_inline)) static inline uint32_t mp_hal_ticks_ms(void)
{
    // Use RTOS ticks for MS timer
    return xTaskGetTickCount();
}
// Task wakeup from ISR
extern void mp_hal_wake_main_task_from_isr(void);

// Using input buffer
extern ringbuf_t stdin_ringbuf;

#define mp_hal_delay_us_fast(us) mp_hal_delay_us(us)

// C-level pin HAL
#include "am_hal_gpio.h"
#include "modmachine.h"

// Not in am_hal_gpio.h?
extern uint32_t ap3_get_pincfg(uint32_t ui32Pin,
                                      am_hal_gpio_pincfg_t *sPincfg);

#define MP_HAL_PIN_FMT "%u"
#define mp_hal_pin_obj_t uint8_t

mp_hal_pin_obj_t machine_pin_get_id(mp_obj_t pin_in);
#define mp_hal_get_pin_obj(o) machine_pin_get_id(o)
#define mp_hal_pin_name(p) (p)

#define mp_hal_pin_read(p) am_hal_gpio_input_read(p)
#define mp_hal_pin_write(p, v) { \
    if (v) { am_hal_gpio_output_set(p); } else { am_hal_gpio_output_clear(p); } \
}


#endif //MICROPY_INCLUDED_APOLLO3_MPHALPORT_H
