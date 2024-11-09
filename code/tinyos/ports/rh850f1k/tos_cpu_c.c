/**
 * @file tos_cpu_c.c
 * @brief cpu dependency functions in C
 * @note sys tick init, task frame init, irq, ...
 */

#include "tos_config.h"
#include "tos_core.h"
#include "tos_core_.h"
#include "tos_cpu.h"


static void tos_task_return(void);


/**
 * @brief OS Tick init
 *
 */
void tos_sys_clock_init(void)
{
    // OSTICK init
}


/**
 * @brief task stack frame init
 *
 * @param proc task proc
 * @param args task arg
 * @param stack_ptr task stack ptr
 * @return tos_stack_t* ptr to current stack
 * @note the stack space and size must be valid,  the stack_ptr pointer to a free valid space
 */
tos_stack_t* tos_task_stack_frame_init(tos_task_proc_t proc, void* args, tos_stack_t* stack_ptr)
{
    uint32_t* reg_tmp = (uint32_t*)stack_ptr;

    *(reg_tmp)   = (uint32_t)0x01010101;        // r1  Assembler-reserved.Used for address generation
    *(--reg_tmp) = (uint32_t)0x02020202;        // r2  Local variable
                                                // r3  Stack pointer -- none
    *(--reg_tmp) = (uint32_t)0x04040404;        // r4  Global pointer for PID
    *(--reg_tmp) = (uint32_t)0x05050505;        // r5  Global pointer for constant data
    *(--reg_tmp) = (uint32_t)args;              // r6  Argument register
    *(--reg_tmp) = (uint32_t)0x07070707;        // r7  Argument register
    *(--reg_tmp) = (uint32_t)0x08080808;        // r8  Argument register
    *(--reg_tmp) = (uint32_t)0x09090909;        // r9  Argument register
    *(--reg_tmp) = (uint32_t)0x10101010;        // r10 Function return value
    *(--reg_tmp) = (uint32_t)0x11111111;        // r11 Working register
    *(--reg_tmp) = (uint32_t)0x12121212;        // r12 Working register
    *(--reg_tmp) = (uint32_t)0x13131313;        // r13 Working register
    *(--reg_tmp) = (uint32_t)0x14141414;        // r14 Working register
    *(--reg_tmp) = (uint32_t)0x15151515;        // r15 Working register
    *(--reg_tmp) = (uint32_t)0x16161616;        // r16 Working register
    *(--reg_tmp) = (uint32_t)0x17171717;        // r17 Working register
    *(--reg_tmp) = (uint32_t)0x18181818;        // r18 Working register
    *(--reg_tmp) = (uint32_t)0x19191919;        // r19 Working register
    *(--reg_tmp) = (uint32_t)0x20202020;        // r20 Used as area for register variable
    *(--reg_tmp) = (uint32_t)0x21212121;        // r21
    *(--reg_tmp) = (uint32_t)0x22222222;        // r22
    *(--reg_tmp) = (uint32_t)0x23232323;        // r23
    *(--reg_tmp) = (uint32_t)0x24242424;        // r24
    *(--reg_tmp) = (uint32_t)0x25252525;        // r25
    *(--reg_tmp) = (uint32_t)0x26262626;        // r26
    *(--reg_tmp) = (uint32_t)0x27272727;        // r27
    *(--reg_tmp) = (uint32_t)0x28282828;        // r28
    *(--reg_tmp) = (uint32_t)0x29292929;        // r29 Used as area for register variable
    *(--reg_tmp) = (uint32_t)0x00303030;        // r30 Element Pointer (EP).
    *(--reg_tmp) = (uint32_t)tos_task_return;   // r31 Link Pointer (LP). function return address
                                                //     task func never return
                                                //     could not be all 0xFF
    *(--reg_tmp) = (uint32_t)proc;              // EIPC
    *(--reg_tmp) = (uint32_t)0x00000000;        // EIPSW interrupt enable
    *(--reg_tmp) = (uint32_t)0x00000000;        // CTPC.
    *(--reg_tmp) = (uint32_t)0x00000000;        // CTPSW.

    return (tos_stack_t*)reg_tmp;
}


/**
 * @brief
 *
 */
static void tos_task_return(void)
{
    tos_task_tcb_t* task = tos_get_current_task();
    tos_task_delete(&task);
    tos_schedule();
    while (1) {
        ;
    }
}
