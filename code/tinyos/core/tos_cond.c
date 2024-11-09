/**
 * @file tos_cond.c
 * @brief condition variable
 * @note
 */
#include "tos_cond.h"
#include "tos_config.h"
#include "tos_core.h"
#include "tos_core_.h"
#include "tos_mem.h"
#include "util_misc.h"
#include "util_queue.h"

#define COND_VALID_FLAG   0x5A5A5A5A
#define COND_INVALID_FLAG 0xFFFFFFFF


typedef struct tos_cond_intenal_t {
    uint32_t          valid_flag;
    uint16_t          use_count;
    uint16_t          value;
    util_queue_node_t waiting_list;
    util_queue_node_t queue_link;   // link conds into list
} tos_cond_intenal_t;


/**
 * @brief
 *
 * @param cond
 * @param attr
 * @return int
 */
int tos_cond_init(tos_cond_t* cond, const tos_cond_attr_t* attr)
{
    if (cond == nullptr) {
        return TOS_ERR_COND_NULLPTR;
    }

    // get a free cond
    tos_cond_intenal_t* cond_intenal = (tos_cond_intenal_t*)tos_malloc(sizeof(tos_cond_intenal_t));
    if (cond_intenal == nullptr) {
        *cond = nullptr;
        return TOS_ERR_COND_NOFREE;
    }

    *cond = cond_intenal;

    cond_intenal->valid_flag = COND_VALID_FLAG;
    cond_intenal->value      = 0;
    cond_intenal->use_count  = 0;
    util_queue_init(&(cond_intenal->waiting_list));

    return 0;
}


/**
 * @brief
 *
 * @param cond
 * @param mutex
 * @return int
 */
int tos_cond_wait(tos_cond_t* cond, tos_mutex_t* mutex)
{
    return tos_cond_waitfor(cond, mutex, TOS_COND_WAIT_INFINITE);
}


/**
 * @brief
 *
 * @param cond
 * @param mutex
 * @param try_nms
 * @return int
 */
int tos_cond_waitfor(tos_cond_t* cond, tos_mutex_t* mutex, uint32_t try_nms)
{
    if (cond == nullptr) {
        return TOS_ERR_COND_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*cond == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_COND_NULLPTR;
    }
    tos_cond_intenal_t* cond_intenal = *cond;

    if (cond_intenal->valid_flag != COND_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_COND_INVALID;
    }
    // get current value
    uint32_t value = cond_intenal->value;
    cond_intenal->use_count++;
    tos_leave_critical_section();

    // unlock mutex, yield
    int ret = tos_mutex_unlock(mutex);
    if (ret != 0) {
        return ret;
    }

    tos_enter_critical_section();

    // cond satisfied
    if (value != cond_intenal->value) {
        cond_intenal->use_count--;
        tos_leave_critical_section();
        tos_mutex_lock(mutex);
        return 0;
    }

    // return immediately
    if (try_nms == TOS_COND_WAIT_IMMEDIATE) {
        cond_intenal->use_count--;
        tos_leave_critical_section();
        return TOS_ERR_COND_TIMEOUT;
    }

    // wait cond
    // add current task to pending list
    tos_task_tcb_t* current_task = tos_get_current_task();
    util_queue_remove(&current_task->ready_pending_link);
    if (util_queue_empty(&tos_state.ready_task_list[current_task->task_prio])) {
        tos_state.ready_task_prio_mask &= ~current_task->task_prio_mask;
    }
    util_queue_insert(&cond_intenal->waiting_list, &current_task->ready_pending_link);

    // add current task into waiting list
    if (try_nms != TOS_COND_WAIT_INFINITE) {
        current_task->task_wait_time = try_nms / TOS_TICK_MS;
        util_queue_insert(&tos_state.waiting_task_list, &current_task->waiting_link);
    }
    tos_leave_critical_section();

    tos_schedule();

    tos_enter_critical_section();

    // cond satisfied
    if (value != cond_intenal->value) {
        cond_intenal->use_count--;
        tos_leave_critical_section();
        tos_mutex_lock(mutex);
        return 0;
    } else {
        cond_intenal->use_count--;
        tos_leave_critical_section();
        return TOS_ERR_COND_TIMEOUT;
    }
}


/**
 * @brief
 *
 * @param cond
 * @return int
 */
int tos_cond_signal(tos_cond_t* cond)
{
    if (cond == nullptr) {
        return TOS_ERR_COND_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*cond == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_COND_NULLPTR;
    }
    tos_cond_intenal_t* cond_intenal = *cond;

    // cond is invalid
    if (cond_intenal->valid_flag != COND_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_COND_INVALID;
    }

    cond_intenal->value++;

    // waiting list is empty
    if (util_queue_empty(&cond_intenal->waiting_list)) {
        tos_leave_critical_section();
        return 0;
    }

    // find task with highest prio
    tos_task_tcb_t*    hignest_prio_task = nullptr;
    uint8_t            hignest_prio      = 0;
    util_queue_node_t* queue_node        = cond_intenal->waiting_list.next;
    while (queue_node != &cond_intenal->waiting_list) {
        tos_task_tcb_t* next_task = get_task_by_ready_pending_link(queue_node);
        if (next_task->task_prio > hignest_prio) {
            hignest_prio_task = next_task;
            hignest_prio      = next_task->task_prio;
        }
        queue_node = queue_node->next;
    }
    if (hignest_prio_task != nullptr) {
        util_queue_remove(&hignest_prio_task->ready_pending_link);   // in blocking list now
        util_queue_remove(&hignest_prio_task->waiting_link);
        util_queue_insert(&tos_state.ready_task_list[hignest_prio_task->task_prio],
                          &hignest_prio_task->ready_pending_link);
        tos_state.ready_task_prio_mask |= hignest_prio_task->task_prio_mask;
    }

    tos_leave_critical_section();

    tos_schedule();

    return 0;
}


/**
 * @brief notify all waiting task
 *
 * @param cond
 * @return int
 */
int tos_cond_broadcast(tos_cond_t* cond)
{
    if (cond == nullptr) {
        return TOS_ERR_COND_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*cond == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_COND_NULLPTR;
    }
    tos_cond_intenal_t* cond_intenal = *cond;

    if (cond_intenal->valid_flag != COND_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_COND_INVALID;
    }

    cond_intenal->value++;

    // waiting list is empty
    if (util_queue_empty(&cond_intenal->waiting_list)) {
        tos_leave_critical_section();
        return 0;
    }

    // notify all task
    while (!util_queue_empty(&cond_intenal->waiting_list)) {
        tos_task_tcb_t* next_task = get_task_by_ready_pending_link(cond_intenal->waiting_list.next);
        util_queue_remove(&next_task->ready_pending_link);   // in blocking list now
        util_queue_remove(&next_task->waiting_link);
        util_queue_insert(&tos_state.ready_task_list[next_task->task_prio], &next_task->ready_pending_link);
        tos_state.ready_task_prio_mask |= next_task->task_prio_mask;
    }
    tos_leave_critical_section();

    tos_schedule();

    return 0;
}


/**
 * @brief
 *
 * @param cond
 * @return int
 */
int tos_cond_destroy(tos_cond_t* cond)
{
    if (cond == nullptr) {
        return TOS_ERR_COND_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*cond == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_COND_NULLPTR;
    }

    tos_cond_intenal_t* cond_intenal = *cond;

    if (cond_intenal->valid_flag != COND_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_COND_INVALID;
    }

    // if (!util_queue_empty(&cond_intenal->waiting_list)) {
    if (cond_intenal->use_count != 0) {
        tos_leave_critical_section();
        return TOS_ERR_COND_BLOCKING;
    }

    cond_intenal->valid_flag = COND_INVALID_FLAG;

    tos_leave_critical_section();

    tos_free(cond_intenal);

    *cond = nullptr;

    return 0;
}
