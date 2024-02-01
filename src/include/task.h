#ifndef __TASK_H_
#define __TASK_H_

#include "device/interrupt.h"
#include "fs/fs.h"
#include "memory/address-space.h"

#define TASK_READY 0
#define TASK_BLOCKED 1
#define TASK_RUNNING 2
#define TASK_DEAD 3

#define TASK_MAX_FILES 128

struct task {
    address_space_t *as;

    uint32_t esp0;

    uint32_t pid;

    file_t *files[TASK_MAX_FILES];

    struct task *next;
    struct task *prev;

    struct task *parent;
};

extern struct task *current_task;

void init_scheduler(void);
int exec(const char *path);
int fork(void);
int exit(int code);

void sleep(void);
void wake(uint32_t pid);
void switch_tasks(void);

void print_regs(registers_t *regs, char *msg);

#endif
