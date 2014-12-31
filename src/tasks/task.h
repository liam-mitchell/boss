#ifndef __TASK_H_
#define __TASK_H_

#include "address-space.h"
#include "interrupt.h"

typedef struct task {
    registers_t regs;
    address_space_t *as;
    uint32_t pid;
    struct task *next;
} task_t;

struct task *run_queue;
struct task *idle;

void init_scheduler();

#endif
