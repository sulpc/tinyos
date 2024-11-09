/**
 * @file tos_mutex.h
 * @brief
 *
 */

#ifndef _TOS_MUTEX_H_
#define _TOS_MUTEX_H_


#include "tos_types.h"


#define TOS_ERR_MUTEX_NULLPTR  -1
#define TOS_ERR_MUTEX_NOFREE   -2
#define TOS_ERR_MUTEX_TIMEOUT  -3
#define TOS_ERR_MUTEX_UNLOCKED -4   // try to unlock an unlocked mutex
#define TOS_ERR_MUTEX_PERM     -5   // permission, not the mutex owner
#define TOS_ERR_MUTEX_BLOCKING -6   // blocking when destroy
#define TOS_ERR_MUTEX_INVALID  -7

#define TOS_TRY_LOCK_INFINITE  0xFFFFFFFFu
#define TOS_TRY_LOCK_IMMEDIATE 0


typedef struct tos_mutex_intenal_t* tos_mutex_t;

typedef struct {
    uint8_t resv;
} tos_mutex_attr_t;


int tos_mutex_init(tos_mutex_t* mutex, const tos_mutex_attr_t* attr);
int tos_mutex_lock(tos_mutex_t* mutex);
int tos_mutex_trylock(tos_mutex_t* mutex, uint32_t try_nms);
int tos_mutex_unlock(tos_mutex_t* mutex);
int tos_mutex_destroy(tos_mutex_t* mutex);


#endif
