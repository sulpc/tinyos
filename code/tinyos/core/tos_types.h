/**
 * @file tos_types.h
 * @brief base types used by tos
 *
 */

#ifndef _TOS_TYPES_H_
#define _TOS_TYPES_H_

#include "util_types.h"

#ifdef _IN_TOS_CORE_C_
#define TOS_EXTERN
#else
#define TOS_EXTERN      extern
#endif


typedef uint32_t        tos_size_t;
typedef uint32_t        tos_stack_t;
typedef void            (*tos_task_proc_t)(void*);

#endif
