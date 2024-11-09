#ifndef _TOS_MEM_H_
#define _TOS_MEM_H_

#include "tos_core.h"
#include "util_heap.h"

static inline void* tos_malloc(tos_size_t size)
{
    tos_use_critical_section();
    tos_enter_critical_section();
    void* p = util_malloc(size);
    tos_leave_critical_section();
    return p;
}

static inline void tos_free(void* ptr)
{
    tos_use_critical_section();
    tos_enter_critical_section();
    util_free(ptr);
    tos_leave_critical_section();
}

#endif
