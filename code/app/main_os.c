#include "bsp.h"
#include "tinyos.h"
#include "utils.h"

static tos_stack_t log_task_stack[512];
static tos_stack_t cli_task_stack[512];
static tos_stack_t usr1_task_stack[512];
static tos_stack_t usr2_task_stack[512];
static tos_stack_t usr3_task_stack[512];

static void bsp_init(void);
static void service_init(void);
static void log_task(void* arg);
static void cli_task(void* arg);
static int  main_cmd_handler(int argc, char* argv[]);
static void usr1_task(void* arg);
static void usr2_task(void* arg);
static void usr3_task(void* arg);

static util_cli_item_t main_cli = {
    .cmd   = "main",
    .help  = "main ...",
    .entry = main_cmd_handler,
};

tos_mutex_t mutex;
tos_cond_t  cond;
int         data = 0;

int main()
{
    bsp_init();
    tos_init();
    service_init();

    // start tos
    util_printk("tos start...\n");
    tos_start();
}

static void bsp_init(void)
{
    sysirq_init();
    uart_console_init(115200);
    util_printk("bsp init ok\n");
}

static void service_init(void)
{
    tos_task_attr_t task;

    task.task_stack_size = sizeof(log_task_stack);
    task.task_prio       = 1;
    task.task_wait_time  = 0;
    task.task_name       = "log";
    task.task_stack      = log_task_stack;
    tos_task_create(log_task, nullptr, &task);

    task.task_stack_size = sizeof(cli_task_stack);
    task.task_prio       = 2;
    task.task_wait_time  = 0;
    task.task_name       = "cli";
    task.task_stack      = cli_task_stack;
    tos_task_create(cli_task, nullptr, &task);
}

static void log_task(void* arg)
{
    while (true) {
        util_log_process();
        tos_task_sleep(10);
    }
}

static void cli_task(void* arg)
{
    util_cli_init();
    util_cli_register(&main_cli);
    while (true) {
        util_cli_process();
        tos_task_sleep(5);
    }
}

static int main_cmd_handler(int argc, char* argv[])
{
    util_printf("main cmd handler\n");

    if (argc == 2 && strcmp(argv[1], "test") == 0) {
        tos_task_attr_t attr;

        tos_mutex_init(&mutex, nullptr);
        tos_cond_init(&cond, nullptr);

        // create task
        attr.task_stack_size = sizeof(usr1_task_stack);
        attr.task_prio       = 1;
        attr.task_wait_time  = 0;
        attr.task_name       = "usr1";
        attr.task_stack      = usr1_task_stack;
        tos_task_create(usr1_task, nullptr, &attr);

        attr.task_stack_size = sizeof(usr2_task_stack);
        attr.task_prio       = 1;
        attr.task_wait_time  = 0;
        attr.task_name       = "usr2";
        attr.task_stack      = usr2_task_stack;
        tos_task_create(usr2_task, nullptr, &attr);

        attr.task_stack_size = sizeof(usr3_task_stack);
        attr.task_prio       = 1;
        attr.task_wait_time  = 0;
        attr.task_name       = "usr3";
        attr.task_stack      = usr3_task_stack;
        tos_task_create(usr3_task, nullptr, &attr);
    }

    return -1;
}

static void usr1_task(void* arg)
{
    static int counter = 1;
    while (true) {
        tos_task_sleep(1000);

        tos_mutex_lock(&mutex);

        util_printf("\n------------------------\n");
        util_printf("user1 get lock\n");
        data = counter++;
        util_printf("user1 produce data %d\n", data);
        tos_mutex_unlock(&mutex);

        tos_task_sleep(1000);

        util_printf("\n");
        util_printf("user1 signal\n");
        tos_cond_signal(&cond);
    }
}

static void usr2_task(void* arg)
{
    while (true) {
        tos_mutex_lock(&mutex);
        util_printf("\n");
        util_printf("user2 get lock\n");
        while (data == 0) {
            util_printf("user2 wait cond\n");
            tos_cond_wait(&cond, &mutex);
        }

        util_printf("\n");
        util_printf("user2 get data %d, consume it.\n", data);
        data = 0;
        tos_mutex_unlock(&mutex);
    }
}

static void usr3_task(void* arg)
{
    while (true) {
        tos_mutex_lock(&mutex);
        util_printf("\n");
        util_printf("user3 get lock\n");
        while (data == 0) {
            util_printf("user3 wait cond\n");
            tos_cond_wait(&cond, &mutex);
        }

        util_printf("\n");
        util_printf("user3 get data %d, consume it.\n", data);
        data = 0;
        tos_mutex_unlock(&mutex);
    }
}
