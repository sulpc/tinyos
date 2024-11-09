/**
 * @file tos_config.h
 * @brief task, stack, clock configs
 *
 */

#ifndef _TOS_CONFIG_H_
#define _TOS_CONFIG_H_

#define TOS_BANNER                                                                                                     \
    " ______ _             ____   ____\n"                                                                              \
    "/_  __/(_)___  __ __ / __ \\ / __/\n"                                                                             \
    " / /  / // _ \\/ // // /_/ /_\\ \\  \n"                                                                           \
    "/_/  /_//_//_/\\_, / \\____//___/  \n"                                                                            \
    "             /___/               \n"

// task config
#define TOS_TASK_NAME_LEN_MAX   16
#define TOS_MAX_PRIO_NUM_USED   8   // max prio is 31
#define TOS_MAX_TASK_NUM_USED   8   // without limit
#define TOS_IDLETASK_STACK_SIZE 512

// clock config
#define TOS_SYS_HZ              1000u
#define TOS_TICK_MS             (1000u / TOS_SYS_HZ)
#define TOS_TIME_WAIT_INFINITY  0xFFFFFFFFu
#define MCU_SYS_CLOCK           72000000u   // 72 MHz

#endif
