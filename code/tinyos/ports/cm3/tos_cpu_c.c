/**
 * @file tos_cpu_c.c
 * @brief cpu dependency functions in C
 * @version 0.1
 * @note task switch, sys tick init, sys irq control
 */

#include "tos_config.h"
#include "tos_core.h"
#include "tos_core_.h"
#include "tos_cpu.h"


/**
 * @brief OS Tick init
 *
 */
void tos_sys_clock_init(void)
{
    uint32_t counts = MCU_SYS_CLOCK / TOS_SYS_HZ;

    *(volatile unsigned int*)0xE000E018  = 0;        // SysTick->VAL
    *(volatile unsigned int*)0xE000E014  = counts;   // SysTick->LOAD
    *(volatile unsigned char*)0xE000ED23 = 0xFF;     // SysTick Prio, lowest

    // SysTick->CTRL
    *(volatile unsigned int*)0xE000E010 |= (1u << 2) | 1u;   // AHB clk, enable systick counter
    *(volatile unsigned int*)0xE000E010 |= (1u << 1);        // enable systick irq
}


/**
 * @brief task stack frame init
 *
 * @param proc task proc
 * @param args task arg
 * @param stack_ptr task stack ptr
 * @return tos_stack_t* ptr to current stack
 * @note the stack space and size must be valid
 */
tos_stack_t* tos_task_stack_frame_init(tos_task_proc_t proc, void* args, tos_stack_t* stack_ptr)
{
    uint32_t* reg_tmp = (uint32_t*)stack_ptr;

    // the reg info restored by cpu when exception occer
    *(reg_tmp)   = (uint32_t)0x01000000;   // xPSR
    *(--reg_tmp) = (uint32_t)proc;         // return addr
    *(--reg_tmp) = (uint32_t)0xFFFFFFFF;   // r14 LR: will not use
    *(--reg_tmp) = (uint32_t)0xCCCCCCCC;   // r12
    *(--reg_tmp) = (uint32_t)0x33333333;   // r3
    *(--reg_tmp) = (uint32_t)0x22222222;   // r2
    *(--reg_tmp) = (uint32_t)0x11111111;   // r1
    *(--reg_tmp) = (uint32_t)args;         // r0 arg

    // the reg info restored by task switch code
    *(--reg_tmp) = (uint32_t)0xBBBBBBBB;   // r11
    *(--reg_tmp) = (uint32_t)0xAAAAAAAA;   // r10
    *(--reg_tmp) = (uint32_t)0x99999999;   // r9
    *(--reg_tmp) = (uint32_t)0x88888888;   // r8
    *(--reg_tmp) = (uint32_t)0x77777777;   // r7
    *(--reg_tmp) = (uint32_t)0x66666666;   // r6
    *(--reg_tmp) = (uint32_t)0x55555555;   // r5
    *(--reg_tmp) = (uint32_t)0x44444444;   // r4

    return (tos_stack_t*)reg_tmp;
}


/**
 * @brief cpu SysTick ISR
 * @note change the default ISR name of stm32
 */
void tos_systick_isr(void)
{
    tos_enter_isr();   // enter ISR
    tos_time_tick();
    tos_exit_isr();   // leave ISR
}
