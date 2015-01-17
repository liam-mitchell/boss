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
    uint32_t state;

    /* file_t *files[TASK_MAX_FILES]; */

    struct task *next;
} task_t;

struct task *run_queue;
struct task *idle;

void init_scheduler();
int exec(const char *path);
int fork();

void print_regs(registers_t *regs, char *msg);

#endif
