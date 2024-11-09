/**
 * @file util_cli.h
 * @author sulpc
 * @brief
 * @version 0.1
 * @date 2024-05-30
 *
 * @copyright Copyright (c) 2024
 *
 * a new version of `srv/shell`
 */
#ifndef _UTIL_CLI_H_
#define _UTIL_CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "util_types.h"

#define CLI_LINE_BUFFER_SIZE 128
#define E_CLI_PARAM_INVALID -100
#define E_CLI_MALLOC_FAIL   -101
#define E_CLI_NOT_INITED    -102

typedef int (*util_cli_entry_t)(int argc, char* argv[]);

typedef struct {
    const char*      cmd;
    util_cli_entry_t entry;
    const char*      help;
} util_cli_item_t;

/**
 * @brief cli init, called first
 *
 */
void util_cli_init(void);

/**
 * @brief
 *
 * @param cli_item must be in static or global address
 * @return int
 */
int util_cli_register(const util_cli_item_t* const cli_item);

/**
 * @brief
 *
 */
void util_cli_process(void);


/**
 * @brief extern function, get a char from console
 *
 * @return int -1: no char, >=0: normal char
 */
extern int console_getc(void);

extern int console_putc(char c);

#ifdef __cplusplus
}
#endif

#endif
