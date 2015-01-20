#ifndef __TASK_H_
#define __TASK_H_

#include "address-space.h"
#include "interrupt.h"
#include "fs.h"

#define TASK_READY 0
#define TASK_BLOCKED 1
#define TASK_RUNNING 2
#define TASK_DEAD 3

#define TASK_MAX_FILES 128

typedef struct task {
    registers_t regs;
    address_space_t *as;

    uint32_t pid;
    /* uint32_t state; */

    file_t *files[TASK_MAX_FILES];

    struct task *next;
    struct task *prev;
} task_t;

struct task *current_task;

void init_scheduler(void);
int exec(const char *path);
int fork(void);

void sleep(void);
void wake(uint32_t pid);

void print_regs(registers_t *regs, char *msg);

#endif
