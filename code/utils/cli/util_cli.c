#include "util_cli.h"
#include "util_heap.h"
#include "util_log.h"
#include "util_misc.h"
#include "util_ringbuffer.h"


#define CLI_USE_HEAP        1
#define CLI_ITEM_NUM_MAX    32
#define CLI_ARG_NUM_MAX     12
#define CLI_PROMPT          "# "
#define cli_printf(...)     util_printf(__VA_ARGS__), util_printf(CLI_PROMPT)


typedef struct cli_list_item_t {
    const util_cli_item_t*  cli_item;
    struct cli_list_item_t* next;
} cli_list_item_t;


static void             cli_cmdline_proc(char* buffer);
static cli_list_item_t* cli_new_list_item(void);
static int              cli_cmd_echo(int argc, char* argv[]);
static int              cli_cmd_help(int argc, char* argv[]);

#if CLI_USE_HEAP == 0
static cli_list_item_t cli_items[CLI_ITEM_NUM_MAX];
static int             cli_item_num = 0;
#endif
static cli_list_item_t* cli_list   = nullptr;
static bool             cli_inited = false;

static util_cli_item_t cli_builtin_cmd[] = {
    {.cmd = "echo", .entry = cli_cmd_echo, .help = "echo"},
    {.cmd = "help", .entry = cli_cmd_help, .help = "list all cmd info"},
};


void util_cli_init(void)
{
#if CLI_USE_HEAP == 0
    cli_item_num = 0;
#endif
    cli_inited = true;
    for (int i = 0; i < util_arraylen(cli_builtin_cmd); i++) {
        util_cli_register(&cli_builtin_cmd[i]);
    }
}


int util_cli_register(const util_cli_item_t* const cli_item)
{
    if (cli_item == nullptr) {
        return E_CLI_PARAM_INVALID;
    }
    if (!cli_inited) {
        return E_CLI_NOT_INITED;
    }

    cli_list_item_t* item = cli_new_list_item();
    if (item == nullptr) {
        return E_CLI_MALLOC_FAIL;
    }

    item->cli_item = cli_item;
    item->next     = cli_list;

    cli_list = item;
    return 0;
}


void util_cli_process(void)
{
    static char line_buffer[CLI_LINE_BUFFER_SIZE];
    static bool over_length = false;
    static int  length      = 0;
    uint8_t     ch;
    int         ret;

    while ((ret = console_getc()) > 0) {
        ch = (uint8_t)(ret & 0xff);
        console_putc(ch);

        if (length >= CLI_LINE_BUFFER_SIZE) {
            // cmdline has invalid length
            length      = 0;
            over_length = true;
        }

        if (ch == '\n') {
            if (over_length) {
                cli_printf("ERROR: cmdline over length, discard it!\n");
                length      = 0;
                over_length = false;
            } else {
                // parse and exec cmdline
                line_buffer[length] = '\0';

                cli_cmdline_proc(line_buffer);

                line_buffer[0] = '\0';   // clear buffer
                length         = 0;

                break;   // exec only one cmd in a period
            }
        } else if (isprint(ch))   // print char
        {
            line_buffer[length] = (char)ch;
            length++;
        } else if (ch == '\t') {   // tab -> ' '
            line_buffer[length] = ' ';
            length++;
        } else if (ch == '\b')   // delete
        {
            if (length > 0) {
                length--;
            }
        } else {
            // ignore other data
        }
    }
}


static cli_list_item_t* cli_new_list_item(void)
{
    cli_list_item_t* item = nullptr;
#if CLI_USE_HEAP == 0
    if (cli_item_num < util_arraylen(cli_items)) {
        item = &cli_items[cli_item_num];
        cli_item_num++;
    }
#else
    item = (cli_list_item_t*)util_malloc(sizeof(cli_list_item_t));
#endif
    return item;
}


static void cli_cmdline_proc(char* buffer)
{
    char*        pchar = buffer;
    static int   argc;
    static char* argv[CLI_ARG_NUM_MAX];

    argc = 0;

    while (*pchar != '\0') {
        // too many args
        if (argc >= CLI_ARG_NUM_MAX) {
            cli_printf("ERROR: too many args (should less than 10)!\n");
            return;
        }
        // jump blanks
        while (*pchar == ' ') {
            pchar++;
        }
        if (*pchar == '\0') {
            break;
        }
        argv[argc] = pchar;
        argc++;

        while (*pchar != ' ' && *pchar != '\0') {
            pchar++;
        }
        if (*pchar == ' ') {
            *pchar = '\0';
            pchar++;
        }
    }

    // empty cmdline
    if (argc == 0) {
        cli_printf("");
        return;
    }

    for (cli_list_item_t* cli_list_item = cli_list; cli_list_item != nullptr; cli_list_item = cli_list_item->next) {
        if (strcmp(cli_list_item->cli_item->cmd, argv[0]) == 0) {
            // find cmd
            cli_list_item->cli_item->entry(argc, argv);
            cli_printf("");
            return;
        }
    }

    // not find
    cli_printf("ERROR: invalid cmd!\n");
    return;
}


static int cli_cmd_echo(int argc, char* argv[])
{
    int i = 0;
    for (i = 1; i < argc; i++) {
        util_printf("%s ", argv[i]);
    }
    util_printf("\n");
    return 0;
}


static int cli_cmd_help(int argc, char* argv[])
{
    for (cli_list_item_t* cli_list_item = cli_list; cli_list_item != nullptr; cli_list_item = cli_list_item->next) {
        util_printf("%s\n    %s\n", cli_list_item->cli_item->cmd, cli_list_item->cli_item->help);
    }
    return 0;
}
