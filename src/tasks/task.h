#ifndef __TASK_H_
#define __TASK_H_

#include "interrupt.h"

typedef struct task {
    registers_t regs;
    address_space_t as;
    uint32_t pid;
    uint32_t parent_pid;
    struct task *next;
} task_t;

task_t *current_task;

task_t *alloc_task();
void schedule_task(task_t *task);
void switch_tasks(registers_t *registers);

void init_scheduler();

#endif // __TASK_H_
