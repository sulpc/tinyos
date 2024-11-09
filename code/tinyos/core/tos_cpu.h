/**
 * @file tos_cpu.h
 * @brief cpu dependency function declaration
 * @note irq, task switch, task frame, systick
 */

#ifndef _TOS_CPU_H_
#define _TOS_CPU_H_

#include "tos_core.h"
#include "tos_types.h"


/**
 * @brief OS Tick init
 *
 */
void tos_sys_clock_init(void);

/**
 * @brief task stack frame init
 *
 * @param proc task proc
 * @param args task arg
 * @param stack_ptr task stack ptr
 * @return tos_stack_t* ptr to current stack
 * @note the stack space and size must be valid
 */
tos_stack_t* tos_task_stack_frame_init(tos_task_proc_t proc, void* args, tos_stack_t* stack_ptr);

/**
 * @brief switch to tos_task_switch_to, store cpu info of tos_task_current
 *
 * @note implement by asm
 */
void tos_task_switch(void);

/**
 * @brief task switch when exit ISR
 *
 * @note implement by asm
 */
void tos_task_switch_intr(void);

/**
 * @brief start first task of TOS, do not store cpu info
 *
 * @note implement by asm
 */
void tos_task_switch_first(void);

/**
 * @brief disable CPU IRQ, return PRIMASK
 *
 * @return uint32_t
 * @note implement by asm
 */
uint32_t tos_irq_diable(void);

/**
 * @brief enable CPU IRQ
 *
 * @note implement by asm
 */
void tos_irq_enable(void);

/**
 * @brief restore CPU IRQ, restore PRIMASK
 *
 * @param primask
 * @note implement by asm
 */
void tos_irq_restore(uint32_t primask);

#define tos_systick_isr SysTick_Handler

#endif
