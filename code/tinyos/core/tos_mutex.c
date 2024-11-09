/**
 * @file tos_mutex.c
 * @brief
 *
 */

#include "tos_mutex.h"
#include "tos_config.h"
#include "tos_core.h"
#include "tos_core_.h"
#include "tos_mem.h"
#include "util_misc.h"
#include "util_queue.h"

#define MUTEX_VALID_FLAG   0x5A5A5A5A
#define MUTEX_INVALID_FLAG 0xFFFFFFFF


typedef struct tos_mutex_intenal_t {
    uint32_t          valid_flag;
    bool              lock_flag;
    tos_task_tcb_t*   owner;
    util_queue_node_t pending_list;
    util_queue_node_t queue_link;
} tos_mutex_intenal_t;


/**
 * @brief
 *
 * @param mutex
 * @param attr
 * @return int
 */
int tos_mutex_init(tos_mutex_t* mutex, const tos_mutex_attr_t* attr)
{
    if (mutex == nullptr) {
        return TOS_ERR_MUTEX_NULLPTR;
    }

    // get a free mutex
    tos_mutex_intenal_t* mutex_intenal = tos_malloc(sizeof(tos_mutex_intenal_t));
    if (mutex_intenal == nullptr) {
        *mutex = nullptr;
        return TOS_ERR_MUTEX_NOFREE;
    }

    *mutex = mutex_intenal;

    mutex_intenal->lock_flag  = false;
    mutex_intenal->valid_flag = MUTEX_VALID_FLAG;
    mutex_intenal->owner      = nullptr;
    util_queue_init(&(mutex_intenal->pending_list));

    return 0;
}


/**
 * @brief
 *
 * @param mutex
 * @return int
 */
int tos_mutex_lock(tos_mutex_t* mutex)
{
    return tos_mutex_trylock(mutex, TOS_TRY_LOCK_INFINITE);
}


/**
 * @brief
 *
 * @param mutex
 * @param try_nms
 * @return int
 */
int tos_mutex_trylock(tos_mutex_t* mutex, uint32_t try_nms)
{
    if (mutex == nullptr) {
        return TOS_ERR_MUTEX_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*mutex == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_NULLPTR;
    }
    tos_mutex_intenal_t* mutex_intenal = *mutex;

    // mutex is invalid
    if (mutex_intenal->valid_flag != MUTEX_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_INVALID;
    }

    // 1 mutex is usable
    if (mutex_intenal->lock_flag == false) {
        mutex_intenal->lock_flag = true;
        mutex_intenal->owner     = tos_get_current_task();   // own task
        tos_leave_critical_section();
        return 0;
    }

    // 2 mutex is locked

    // 2.1 immediately
    if (try_nms == TOS_TRY_LOCK_IMMEDIATE) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_TIMEOUT;
    }

    // 2.2 wait mutex
    // add current task to pending list
    tos_task_tcb_t* current_task = tos_get_current_task();
    util_queue_remove(&current_task->ready_pending_link);
    if (util_queue_empty(&tos_state.ready_task_list[current_task->task_prio])) {
        tos_state.ready_task_prio_mask &= ~current_task->task_prio_mask;
    }
    util_queue_insert(&mutex_intenal->pending_list, &current_task->ready_pending_link);

    // add current task into waiting list
    if (try_nms != TOS_TRY_LOCK_INFINITE) {
        current_task->task_wait_time = try_nms / TOS_TICK_MS;
        util_queue_insert(&tos_state.waiting_task_list, &current_task->waiting_link);
    }
    tos_leave_critical_section();

    tos_schedule();

    return (mutex_intenal->owner == current_task) ? 0 : TOS_ERR_MUTEX_TIMEOUT;
}


/**
 * @brief
 *
 * @param mutex
 * @return int
 */
int tos_mutex_unlock(tos_mutex_t* mutex)
{
    if (mutex == nullptr) {
        return TOS_ERR_MUTEX_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*mutex == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_NULLPTR;
    }
    tos_mutex_intenal_t* mutex_intenal = *mutex;

    // mutex is invalid
    if (mutex_intenal->valid_flag != MUTEX_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_INVALID;
    }

    // mutex is unlock
    if (mutex_intenal->lock_flag == false) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_UNLOCKED;
    }

    // mutex not own by current task
    if (mutex_intenal->owner != tos_get_current_task()) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_PERM;
    }

    // pending list is empty
    if (util_queue_empty(&mutex_intenal->pending_list)) {
        mutex_intenal->lock_flag = false;
        mutex_intenal->owner     = nullptr;
        tos_leave_critical_section();
        return 0;
    }

    // active next pending task
    tos_task_tcb_t* next_task = get_task_by_ready_pending_link(mutex_intenal->pending_list.next);
    util_queue_remove(&next_task->ready_pending_link);   // in blocking list now
    util_queue_remove(&next_task->waiting_link);
    util_queue_insert(&tos_state.ready_task_list[next_task->task_prio], &next_task->ready_pending_link);
    tos_state.ready_task_prio_mask |= next_task->task_prio_mask;
    mutex_intenal->owner = next_task;

    tos_leave_critical_section();

    tos_schedule();

    return 0;
}


/**
 * @brief
 *
 * @param mutex
 * @return int
 */
int tos_mutex_destroy(tos_mutex_t* mutex)
{
    if (mutex == nullptr) {
        return TOS_ERR_MUTEX_NULLPTR;
    }

    tos_use_critical_section();
    tos_enter_critical_section();

    if (*mutex == nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_NULLPTR;
    }
    tos_mutex_intenal_t* mutex_intenal = *mutex;

    // mutex is invalid
    if (mutex_intenal->valid_flag != MUTEX_VALID_FLAG) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_INVALID;
    }

    // mutex is locking or pending
    // if (!util_queue_empty(&mutex_intenal->pending_list) || mutex_intenal->lock_flag == true) {
    if (mutex_intenal->owner != nullptr) {
        tos_leave_critical_section();
        return TOS_ERR_MUTEX_BLOCKING;
    }

    mutex_intenal->valid_flag = MUTEX_INVALID_FLAG;

    tos_leave_critical_section();

    tos_free(mutex_intenal);

    *mutex = nullptr;

    return 0;
}
