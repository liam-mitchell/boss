#include "task.h"

#include "kheap.h"

static uint32_t next_pid;

static task_t *alloc_task()
{
    task_t *task = kzalloc(sizeof(*task));
    task->as = alloc_address_space();
    task->pid = next_pid++;
    return task;
}

void init_scheduler()
{
    idle = alloc_task();
    run_queue = alloc_task();
}
