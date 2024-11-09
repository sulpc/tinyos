// tos_cpu_s.s

.syntax unified
.thumb
.text

// Declaration Extern Functions and Variables

.extern tos_task_current
.extern tos_task_switch_to
.extern tos_task_prio_current
.extern tos_task_prio_switch_to

.global tos_irq_diable
.global tos_irq_enable
.global tos_irq_restore
.global tos_task_switch_first
.global tos_task_switch
.global tos_task_switch_intr
.global PendSV_Handler                          // use the default ISR name

.equ NVIC_PendSV_Prio_Reg, 0xE000ED22
.equ NVIC_PendSV_Prio,     0xFF                 // lowest
.equ NVIC_Int_Ctrl_Reg,    0xE000ED04
.equ NVIC_PendSV_Set,      0x10000000           // trigger PendSV


// in Thumb mode, extern function must use `.thumb_func` pseudo instruction


.thumb_func
tos_irq_diable:
    mrs     r0, primask                         // return PRIMASK
    cpsid   i                                   // set PRIMASK to disable IRQ
    bx      lr


.thumb_func
tos_irq_enable:
    cpsie   i                                   // clr PRIMASK to enable IRQ
    bx      lr


.thumb_func
tos_irq_restore:
    msr     primask, r0                         // restore PRIMASK
    // cpsie   i                                // restore to prev status (enable or disable)
    bx      lr


.thumb_func
tos_task_switch_first:
    // set PendSV Prio
    ldr     r0, =NVIC_PendSV_Prio_Reg
    ldr     r1, =NVIC_PendSV_Prio
    strb    r1, [R0]

    // Set the PSP to 0 for initial context switch call
    movs    r0, #0
    msr     psp, r0

    // Trigger the PendSV exception (causes context switch)
    ldr     r0, =NVIC_Int_Ctrl_Reg
    ldr     r1, =NVIC_PendSV_Set
    str     r1, [r0]
    cpsie   i                                   // enable irq (disabled in tos_init), trigger PendSV (task switch)
    // bx      lr                               // switch to first task, this function will never return

tos_start_error:
    b       tos_start_error                     // Should never get here


.thumb_func
tos_task_switch:
    ldr     r0, =NVIC_Int_Ctrl_Reg              // trigger PendSV (task switch)
    ldr     r1, =NVIC_PendSV_Set
    str     r1, [r0]
    bx      lr


.thumb_func
tos_task_switch_intr:
    ldr     r0, =NVIC_Int_Ctrl_Reg              // trigger PendSV (task switch)
    ldr     r1, =NVIC_PendSV_Set
    str     r1, [r0]
    bx      lr


.thumb_func
PendSV_Handler:
    cpsid   i

    // cannot use psp in int handler
    mrs     r0, psp
    cbz     r0, PendSV_Handler_WithoutSave      // first use
    subs    r0, r0, #0x20
    stm     r0, {r4-r11}

    // save the psp
    ldr     r1, =tos_task_current
    ldr     r1, [r1]
    str     r0, [r1]

PendSV_Handler_WithoutSave:
    // tos_task_prio_current <= tos_task_prio_switch_to
    ldr     r0, =tos_task_prio_switch_to
    ldr     r1, =tos_task_prio_current
    ldr     r2, [r0]
    str     r2, [r1]

    // tos_task_current <= tos_task_switch_to
    ldr     r0, =tos_task_switch_to
    ldr     r1, =tos_task_current
    ldr     r2, [r0]
    str     r2, [r1]  

    // r2 point to taskSwitchTo
    // change the psp
    ldr     r0, [r2]
    ldm     r0, {r4-r11}
    adds    r0, r0, #0x20
    msr     psp, r0

    orr     lr, lr, #0x04

    cpsie   i
    bx      lr

    // nop                                      // align to 2 bytes
.end
