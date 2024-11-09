/**
 * @file tos_core.c
 * @brief tos core and task management module
 */

#define _IN_TOS_CORE_C_


#include "tos_core.h"
#include "tos_config.h"
#include "tos_core_.h"
#include "tos_mem.h"
#include "util_log.h"
#include "util_misc.h"


static uint32_t        tos_get_highest_prio(uint32_t prio_mask);
static void            tos_idle_task_proc(void* args);
static tos_task_tcb_t* tos_get_free_tcb(void);
static tos_task_tcb_t* tos_task_tcb_init(tos_task_attr_t* attr, tos_stack_t* task_stack_ptr);


uint32_t           tos_task_prio_current;                                                // task core cpu
uint32_t           tos_task_prio_switch_to;                                              // core cpu
tos_task_tcb_t*    tos_task_current;                                                     //
tos_task_tcb_t*    tos_task_switch_to;                                                   //
tos_run_state_t    tos_state = {0};                                                      //
static tos_stack_t tos_idle_task_stack[TOS_IDLETASK_STACK_SIZE / sizeof(tos_stack_t)];   //


bool tos_init(void)
{
    uint32_t        index;
    tos_task_attr_t task_attr;

    tos_irq_diable();   // irq will be enable in tos_start

    // state init
    tos_task_current          = nullptr;
    tos_task_prio_current     = 0;
    tos_state.intr_level      = 0;
    tos_state.schedule_enable = false;
    tos_state.sys_running     = false;
    tos_state.sys_ticks       = 0;
    tos_state.task_number     = 0;

    // init ready_list, waiting_list, all_list
    tos_state.ready_task_prio_mask = 0;
    for (index = 0; index <= TOS_MAX_PRIO_NUM_USED; index++) {
        util_queue_init(&tos_state.ready_task_list[index]);
    }
    util_queue_init(&tos_state.waiting_task_list);
    util_queue_init(&tos_state.all_task_list);

    // create idle task
    task_attr.task_name       = "idle";
    task_attr.task_wait_time  = 0;
    task_attr.task_prio       = 0;   // lowest prio
    task_attr.task_stack_size = TOS_IDLETASK_STACK_SIZE;
    task_attr.task_stack      = tos_idle_task_stack;

    if (tos_task_create(tos_idle_task_proc, nullptr, &task_attr) == nullptr) {
        // tos_error("idle task create error!");
        return false;
    }

    return true;
}


void tos_start(void)
{
    util_printk(TOS_BANNER);

    tos_task_prio_switch_to = tos_get_highest_prio(tos_state.ready_task_prio_mask);
    tos_task_switch_to      = get_task_by_ready_pending_link(tos_state.ready_task_list[tos_task_prio_switch_to].next);

    tos_state.sys_running     = true;
    tos_state.schedule_enable = true;

    tos_task_switch_to->task_switch_cnt++;

    tos_sys_clock_init();   // tos sys tick clock init

    // CM3 use PendSV IRQ to switch task, use a flag to tell the PendSV ISR if it need to store the context
    // for other platform, call the `switch_to` asm code derectly
    tos_task_switch_first();
}


void tos_clock_start(void)
{
    tos_sys_clock_init();   // tos sys tick clock init
}


void tos_enter_isr(void)
{
    tos_use_critical_section();

    tos_enter_critical_section();
    if (tos_state.sys_running) {
        if (tos_state.intr_level < 255) {
            tos_state.intr_level++;
        }
    }

    tos_leave_critical_section();
}


void tos_exit_isr(void)
{
    tos_use_critical_section();

    if (tos_state.sys_running == true) {
        tos_enter_critical_section();

        if (tos_state.intr_level > 0)
            tos_state.intr_level--;

        // schedule when all intr exit
        if (tos_state.intr_level == 0 && tos_state.schedule_enable == true) {
            tos_task_prio_switch_to = tos_get_highest_prio(tos_state.ready_task_prio_mask);
            tos_task_switch_to      = get_task_by_ready_pending_link(tos_state.ready_task_list[tos_task_prio_switch_to].next);

            if (tos_task_switch_to != tos_task_current) {
                tos_task_switch_to->task_switch_cnt++;
                tos_task_switch_intr();
            }
        }

        tos_leave_critical_section();
    }
}


void tos_schedule_enable(void)
{
    tos_use_critical_section();

    tos_enter_critical_section();

    tos_state.schedule_enable = true;

    tos_leave_critical_section();
}


void tos_schedule_disable(void)
{
    tos_use_critical_section();

    tos_enter_critical_section();

    tos_state.schedule_enable = false;

    tos_leave_critical_section();
}


tos_task_t tos_task_create(tos_task_proc_t proc, void* args, tos_task_attr_t* attr)
{
    tos_use_critical_section();

    if (attr == nullptr || attr->task_stack == nullptr) {
        return nullptr;
    }

    tos_enter_critical_section();
    if (tos_state.intr_level > 0) {
        tos_leave_critical_section();
        return nullptr;
    }

    tos_leave_critical_section();

    tos_stack_t* stack_end = &attr->task_stack[(attr->task_stack_size / sizeof(tos_stack_t))];

    util_printk("create %15s: [%p, %p) %4d Bytes.\n", attr->task_name, attr->task_stack, stack_end,
                attr->task_stack_size);

    tos_stack_t*    task_stack_ptr = tos_task_stack_frame_init(proc, args, stack_end - 1);
    tos_task_tcb_t* tcb            = tos_task_tcb_init(attr, task_stack_ptr);

    if (tcb == nullptr) {
        // tos_error("try to create task(%s, id=%d) error", attr->task_name, tos_state.task_number + 1);
        return nullptr;
    }

    if (tos_state.sys_running) {
        tos_schedule();
    }

    return tcb;
}


void tos_task_delete(tos_task_t* task)
{
    if (task == nullptr)
        return;

    tos_task_tcb_t* tcb = *task;

    tos_use_critical_section();

    // if (tos_state.intr_level > 0 || tcb == tos_task_current)
    if (tos_state.intr_level > 0) {
        return;
    }

    tos_enter_critical_section();

    // remove task from read_pending list
    util_queue_remove(&tcb->ready_pending_link);
    if (util_queue_empty(&tos_state.ready_task_list[tcb->task_prio])) {
        tos_state.ready_task_prio_mask &= ~tcb->task_prio_mask;
    }
    // remove task from waiting list
    util_queue_remove(&tcb->waiting_link);

    // remove task from all_task list
    tos_state.task_number--;
    util_queue_remove(&tcb->all_link);

    tos_leave_critical_section();

    tos_free(tcb);
    *task = nullptr;
}


int32_t tos_set_task_prio(tos_task_t* task, uint32_t prio)
{
    if (task == nullptr)
        return -1;

    tos_task_tcb_t* tcb = *task;

    tos_use_critical_section();

    uint8_t prio_old = tcb->task_prio;

    if (prio == tcb->task_prio) {
        return -1;
    }

    // running or ready task, in read_pending list
    if (tcb->task_state == TOS_TASK_STATE_RUNNING || tcb->task_state == TOS_TASK_STATE_READY) {
        tos_enter_critical_section();

        // remove from old list
        util_queue_remove(&tcb->ready_pending_link);
        if (util_queue_empty(&tos_state.ready_task_list[tcb->task_prio])) {
            tos_state.ready_task_prio_mask &= ~tcb->task_prio_mask;
        }

        // add into new list
        util_queue_insert(&tos_state.ready_task_list[prio], &tcb->ready_pending_link);
        tos_state.ready_task_prio_mask |= 1 << prio;

        tos_leave_critical_section();
    }

    // sleep or block task, in waiting or sem list, modify prio immediately
    tcb->task_prio      = prio;
    tcb->task_prio_mask = 1 << prio;

    if (tcb == tos_task_current) {
        tos_enter_critical_section();

        tos_task_prio_current = tcb->task_prio_mask;

        tos_leave_critical_section();
    }

    tos_schedule();

    return prio_old;
}


int32_t tos_get_task_prio(tos_task_t* task)
{
    if (task == nullptr)
        return -1;

    tos_task_tcb_t* tcb = *task;

    return tcb->task_prio;
}


tos_task_state_t tos_get_task_state(tos_task_t* task)
{
    if (task == nullptr)
        return TOS_TASK_INVALID;

    tos_task_tcb_t* tcb = *task;

    return tcb->task_state;
}


void tos_task_sleep(uint32_t nms)
{
    if (nms == 0)
        nms = 1;

    tos_use_critical_section();

    if (tos_state.intr_level > 0 || nms == 0) {
        return;
    }

    tos_enter_critical_section();

    util_queue_remove(&tos_task_current->ready_pending_link);
    if (util_queue_empty(&tos_state.ready_task_list[tos_task_current->task_prio])) {
        tos_state.ready_task_prio_mask &= ~tos_task_current->task_prio_mask;
    }
    util_queue_init(&tos_task_current->ready_pending_link);

    tos_task_current->task_wait_time = nms / TOS_TICK_MS;
    util_queue_insert(&tos_state.waiting_task_list, &tos_task_current->waiting_link);

    tos_leave_critical_section();

    tos_schedule();
}


void tos_task_yield(void)
{
    tos_use_critical_section();
    // important to use critical_section
    // or maybe conflict with systick ISR

    tos_enter_critical_section();
    // remove from ready list, then add into list tail
    util_queue_remove(&tos_task_current->ready_pending_link);
    util_queue_insert(&tos_state.ready_task_list[tos_task_current->task_prio], &tos_task_current->ready_pending_link);
    tos_leave_critical_section();

    tos_schedule();
}


bool tos_running(void)
{
    return tos_state.sys_running;
}


tos_task_tcb_t* tos_get_current_task(void)
{
    return tos_task_current;
}


void tos_time_tick(void)
{
    util_queue_node_t* list_node;
    util_queue_node_t* list_next;
    tos_task_tcb_t*    tcb;
    tos_use_critical_section();

    tos_state.sys_ticks++;
    tos_enter_critical_section();

    // travel the time wait list
    for (list_node = tos_state.waiting_task_list.next; list_node != &tos_state.waiting_task_list; list_node = list_next) {
        tcb       = get_task_by_waiting_link(list_node);
        list_next = list_node->next;   // NOTICE: list link maybe changed in the loop

        if (tcb->task_wait_time != TOS_TIME_WAIT_INFINITY) {
            if (--tcb->task_wait_time == 0) {
                // move the task to ready list
                util_queue_remove(&tcb->ready_pending_link);   // the task may block in a sem list
                util_queue_insert(&tos_state.ready_task_list[tcb->task_prio], &tcb->ready_pending_link);
                tos_state.ready_task_prio_mask |= tcb->task_prio_mask;

                util_queue_remove(&tcb->waiting_link);
                util_queue_init(&tcb->waiting_link);   // make list_node->next==list_node
            }
        }
    }

    tos_leave_critical_section();
}


void tos_schedule(void)
{
    tos_use_critical_section();

    tos_enter_critical_section();

    if (tos_state.intr_level == 0 && tos_state.schedule_enable == true) {
        tos_task_prio_switch_to = tos_get_highest_prio(tos_state.ready_task_prio_mask);
        tos_task_switch_to = get_task_by_ready_pending_link(tos_state.ready_task_list[tos_task_prio_switch_to].next);

        if (tos_task_switch_to != tos_task_current) {
            tos_task_switch_to->task_switch_cnt++;
            tos_task_switch();
        }
    }

    tos_leave_critical_section();
}


/**
 * @brief init task TCB
 *
 * @param attr taskattr
 * @param task_stack_ptr ptr to task stack
 * @return tos_task_tcb_t*
 */
static tos_task_tcb_t* tos_task_tcb_init(tos_task_attr_t* attr, tos_stack_t* task_stack_ptr)
{
    tos_task_tcb_t* tcb;
    tos_use_critical_section();

    tcb = tos_get_free_tcb();

    if (tcb == nullptr) {
        return nullptr;
    }

    // task stack: stack is high addr to low addr
    tcb->task_stk_ptr  = task_stack_ptr;
    tcb->task_stk_base = &attr->task_stack[(attr->task_stack_size / sizeof(tos_stack_t)) - 1];
    tcb->task_stk_size = attr->task_stack_size;
    tcb->task_stk_top  = attr->task_stack - 1;

    // schdule and other addr
    tcb->task_prio        = attr->task_prio;
    tcb->task_prio_mask   = 1u << attr->task_prio;
    tcb->task_wait_time   = attr->task_wait_time;
    tcb->task_id          = tos_state.task_number;
    tcb->task_state       = TOS_TASK_STATE_READY;
    tcb->task_switch_cnt  = 0;
    tcb->task_total_ticks = 0;
    tcb->task_flag        = 0;
    strncmp(tcb->task_name, attr->task_name, TOS_TASK_NAME_LEN_MAX - 1);
    tcb->task_name[TOS_TASK_NAME_LEN_MAX - 1] = 0;

    // modify global var, enter critical section
    tos_enter_critical_section();

    // inset new task tcb to ready list
    util_queue_insert(&tos_state.ready_task_list[tcb->task_prio], &tcb->ready_pending_link);
    util_queue_init(&tcb->waiting_link);

    tos_state.ready_task_prio_mask |= tcb->task_prio_mask;
    tos_state.task_number++;
    util_queue_insert(&tos_state.all_task_list, &tcb->all_link);

    tos_leave_critical_section();

    return tcb;
}


/**
 * get a free tcb
 *
 *  @param   void
 *  @return  a free tcb, or nullptr
 *  @see
 *  @note
 */
static tos_task_tcb_t* tos_get_free_tcb(void)
{
    return (tos_task_tcb_t*)tos_malloc(sizeof(tos_task_tcb_t));
}


/**
 * @brief get highest prio
 *
 * @param prio_mask
 * @return uint32_t
 */
static uint32_t tos_get_highest_prio(uint32_t prio_mask)
{
    uint32_t prio = 31;
    while ((prio_mask & (1u << prio)) == 0) {
        prio--;
    }
    return prio;
}


/**
 * @brief idle task proc
 *
 * @param args
 */
static void tos_idle_task_proc(void* args)
{
    while (true) {
    }
}
