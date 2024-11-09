/**
 * @file tos_cond.h
 * @brief condition variable
 * @note
 */

#ifndef _TOS_COND_H_
#define _TOS_COND_H_


#include "tos_mutex.h"
#include "tos_types.h"


#define TOS_ERR_COND_NULLPTR    -1
#define TOS_ERR_COND_NOFREE     -2
#define TOS_ERR_COND_TIMEOUT    -3
#define TOS_ERR_COND_UNLOCKED   -4
#define TOS_ERR_COND_PERM       -5   // permission, not the mutex owner
#define TOS_ERR_COND_BLOCKING   -6   // blocking when destroy
#define TOS_ERR_COND_INVALID    -7

#define TOS_COND_WAIT_INFINITE  0xFFFFFFFFu
#define TOS_COND_WAIT_IMMEDIATE 0


typedef struct tos_cond_intenal_t* tos_cond_t;
typedef struct {
    uint8_t resv;
} tos_cond_attr_t;


/**
 * @brief
 *
 * @param cond
 * @param attr
 * @return int
 */
int tos_cond_init(tos_cond_t* cond, const tos_cond_attr_t* attr);

/**
 * @brief
 *
 * @param cond
 * @param mutex
 * @return int
 */
int tos_cond_wait(tos_cond_t* cond, tos_mutex_t* mutex);

/**
 * @brief
 *
 * @param cond
 * @param mutex
 * @param try_nms
 * @return int
 */
int tos_cond_waitfor(tos_cond_t* cond, tos_mutex_t* mutex, uint32_t try_nms);

/**
 * @brief
 *
 * @param cond
 * @return int
 */
int tos_cond_signal(tos_cond_t* cond);

/**
 * @brief notify all waiting task
 *
 * @param cond
 * @return int
 */
int tos_cond_broadcast(tos_cond_t* cond);


/**
 * @brief
 *
 * @param cond
 * @return int
 */
int tos_cond_destroy(tos_cond_t* cond);

#endif
