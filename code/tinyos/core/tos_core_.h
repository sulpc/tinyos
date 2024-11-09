/**
 * @file tos_core_.h
 * @brief tos core and task management module
 * @note private, not for user
 */

#ifndef _TOS_CORE__H_
#define _TOS_CORE__H_


#include "tos_config.h"
#include "tos_core.h"
#include "util_queue.h"


#define get_task_by_ready_pending_link(link)                                                                           \
    ((tos_task_tcb_t*)((uint8_t*)(link) - (uint32_t) & ((tos_task_tcb_t*)0)->ready_pending_link))
#define get_task_by_waiting_link(link)                                                                                 \
    ((tos_task_tcb_t*)((uint8_t*)(link) - (uint32_t) & ((tos_task_tcb_t*)0)->waiting_link))
#define get_task_by_all_link(link) ((tos_task_tcb_t*)((uint8_t*)(link) - (uint32_t) & ((tos_task_tcb_t*)0)->all_link))

typedef struct tos_task_tcb_t {
    tos_stack_t*      task_stk_ptr;         // stack ptr
    tos_stack_t*      task_stk_base;        // stack base addr (highest mostly)
    tos_stack_t*      task_stk_top;         // stack top addr (next addr after stack space)
    uint32_t          task_stk_size;        // stack size
    util_queue_node_t ready_pending_link;   // link to ready list or pending task list
    util_queue_node_t waiting_link;         // link to time waiting task list
    util_queue_node_t all_link;             // link to all task list
    uint32_t          task_prio_mask;       // same as (1 << prio)
    uint32_t          task_wait_time;       // sleep or wait
    uint32_t          task_id;
    uint32_t          task_switch_cnt;
    uint32_t          task_total_ticks;
    uint32_t          task_flag;
    uint8_t           task_prio;
    tos_task_state_t  task_state;
    char              task_name[TOS_TASK_NAME_LEN_MAX];   // name info
} tos_task_tcb_t;

typedef struct {
    uint32_t          task_number;                                  // valid task number
    uint32_t          intr_level;                                   //
    uint32_t          sys_ticks;                                    //
    uint32_t          ready_task_prio_mask;                         //
    bool              schedule_enable;                              //
    bool              sys_running;                                  //
    util_queue_node_t all_task_list;                                // all tasks
    util_queue_node_t waiting_task_list;                            // time waiting tasks
    util_queue_node_t ready_task_list[TOS_MAX_PRIO_NUM_USED + 1];   // ready tasks (like hash table)
} tos_run_state_t;


extern tos_run_state_t tos_state;


/**
 * @brief
 *
 * @return tos_task_tcb_t*
 */
tos_task_tcb_t* tos_get_current_task(void);

/**
 * @brief
 * @note called by cpu timer ISR
 */
void tos_time_tick(void);

/**
 * @brief tos_schedule
 *
 */
void tos_schedule(void);


#endif
