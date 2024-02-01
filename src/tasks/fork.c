#include "task.h"
#include "internal.h"

#include "compiler.h"
#include "errno.h"

#include "memory/address-space.h"
#include "memory/kheap.h"
#include "memory/memory.h"

static uint32_t clone_kstack(uint32_t esp0)
{
    uint32_t stack_start = align_down(esp0, PAGE_SIZE);
    uint32_t stack_offset = esp0 - stack_start;

    void *new = kmalloc(MEM_GEN, PAGE_SIZE);
    if (!new) {
        return 0;
    }

    memcpy(new, (void *)stack_start, PAGE_SIZE);
    return (uint32_t)new + stack_offset;
}

int fork(void)
{
    int err = 0;

    struct task *child = alloc_task();
    if (!child) {
        err = -ENOMEM;
        goto error;
    }

    child->as = clone_address_space();
    if (!child->as) {
        err = -ENOMEM;
        goto error;
    }

    child->esp0 = clone_kstack(current_task->esp0);
    if (!child->esp0) {
        err = -ENOMEM;
        goto error;
    }

    registers_t *child_regs = (registers_t *)child->esp0;
    child_regs->eax = 0;

    child->parent = current_task;

    task_queue_add(&running, child);

    return child->pid;

 error:
    free_task(child);
    return err;
}

