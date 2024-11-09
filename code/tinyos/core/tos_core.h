
/**
 * @file tos_core.h
 * @brief tos core and task management module
 *
 */

#ifndef _TOS_CORE_H_
#define _TOS_CORE_H_


#include "tos_cpu.h"
#include "tos_types.h"


#define tos_use_critical_section() uint32_t cpu_primask

// store PRIMASK, close irq
#define tos_enter_critical_section()                                                                                   \
    do {                                                                                                               \
        cpu_primask = tos_irq_diable();                                                                                \
    } while (0)

// restore PRIMASK
#define tos_leave_critical_section()                                                                                   \
    do {                                                                                                               \
        tos_irq_restore(cpu_primask);                                                                                  \
    } while (0)

typedef struct tos_task_tcb_t* tos_task_t;   // handle of task

typedef enum {
    TOS_TASK_INVALID = 0x00,   //
    TOS_TASK_STATE_RUNNING,    // running now
    TOS_TASK_STATE_READY,      // ready but not scheduled yet
    TOS_TASK_STATE_PENDING,    // pending in mutex or sem
    TOS_TASK_STATE_WAITING,    // sleep for some time
} tos_task_state_t;

typedef struct {
    tos_stack_t* task_stack;        // begin address of task area (lowest addr mostly, 4B align)
    uint32_t     task_stack_size;   // n bytes
    uint8_t      task_prio;
    uint32_t     task_wait_time;
    char*        task_name;
} tos_task_attr_t;


/**
 * @brief init variables used by tos
 *
 * @return true
 * @return false
 */
bool tos_init(void);

/**
 * @brief start tos
 * @note called after tos_init
 */
void tos_start(void);

/**
 * @brief start tos clock
 * @note much better to call it in init_task
 */
void tos_clock_start(void);

/**
 * @brief called at begin of ISR
 *
 */
void tos_enter_isr(void);

/**
 * @brief called at end of ISR
 *
 */
void tos_exit_isr(void);

/**
 * @brief enable schedule
 *
 */
void tos_schedule_enable(void);

/**
 * @brief disable schedule
 *
 */
void tos_schedule_disable(void);

/**
 * @brief create new task
 *
 * @param proc task proc
 * @param args args of task proc
 * @param attr task attr
 * @return handle of task, return nullptr when failed
 */
tos_task_t tos_task_create(tos_task_proc_t proc, void* args, tos_task_attr_t* attr);

/**
 * @brief delete task
 *
 * @param task
 */
void tos_task_delete(tos_task_t* task);

/**
 * @brief
 *
 * @param task
 * @param prio
 * @return int32_t old prio when succeed, -1 when fail
 */
int32_t tos_set_task_prio(tos_task_t* task, uint32_t prio);

/**
 * @brief
 *
 * @param task
 * @return int32_t
 */
int32_t tos_get_task_prio(tos_task_t* task);

/**
 * @brief get task state
 *
 * @param task
 * @return tos_task_state_t
 */
tos_task_state_t tos_get_task_state(tos_task_t* task);

/**
 * @brief task sleep some time
 *
 * @param nms
 */
void tos_task_sleep(uint32_t nms);

/**
 * @brief task yield
 *
 */
void tos_task_yield(void);

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool tos_running(void);

#endif
