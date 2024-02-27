#ifndef __TASK_H_
#define __TASK_H_

#include "device/interrupt.h"
#include "fs/fs.h"
#include "memory/address-space.h"
#include "list.h"

#define TASK_MAX_FILES 128

enum status {
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_FINISHED
};

struct task {
    address_space_t *as;

    uint32_t esp0;

    uint32_t pid;
    enum status status;

    file_t *files[TASK_MAX_FILES];

    struct task *next;
    struct task *prev;

    struct task *parent;

    struct list children; // head of our list of children
    struct list children_list; // our node in the list of our parent's children

    int exit_code;
};

extern struct task *current_task;

void init_scheduler(void);
int exec(const char __user *path);
int fork(void);
void exit(int code);
int wait(uint32_t pid, int __user *status);

void sleep(void);
void wake(uint32_t pid);
void switch_tasks(void);

void print_regs(registers_t *regs, char *msg);

#endif
