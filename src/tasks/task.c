#include "task.h"
#include "internal.h"

#include "algorithm.h"
#include "bool.h"
#include "errno.h"
#include "ldsymbol.h"
#include "printf.h"

#include "device/descriptor_tables.h"
#include "device/port.h"
#include "device/timer.h"
#include "fs/vfs.h"
#include "memory/kheap.h"
#include "memory/memory.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

extern ldsymbol ld_virtual_offset;

static uint32_t next_pid;

struct task *current_task;

struct task *running;
struct task *blocked;
struct task *idle;
struct task *zombies;

/**
 * Task queue functions. Implemented with a simple doubly linked list.
 *
 * Removing doesn't work strictly as a queue - tasks can be found and
 * removed anywhere in the queue by pid.
 */
void task_queue_add(struct task **queue, struct task *task)
{
    if (!task) {
        return;
    }
    
    struct task *curr = *queue;

    if (!curr) {
        *queue = task;
        task->next = NULL;
        task->prev = NULL;
        return;
    }

    while (curr->next) {
        curr = curr->next;
    }

    curr->next = task;
    task->prev = curr;
    task->next = NULL;
}

void task_queue_remove(struct task **queue, struct task *task)
{
    if (!task) {
        return;
    }

    if (*queue == task) {
        *queue = task->next;
    }

    if (task->prev) {
        task->prev->next = task->next;
    }

    if (task->next) {
        task->next->prev = task->prev;
    }
}

struct task *task_queue_find(struct task **queue, uint32_t pid)
{
    for (struct task *task = *queue; task; task = task->next) {
        if (task->pid == pid) {
            return task;
        }
    }

    return NULL;
}

void task_queue_remove_safe(struct task **queue, struct task *task)
{
    struct task *found = task_queue_find(queue, task->pid);
    if (found) {
	printf("removing task from queue %s\n", (queue == &running ? "running" : (queue == &blocked ? "blocked" : "zombies")));
	task_queue_remove(queue, task);
    }
}

/**
 * Management of task memory, pids and default files, etc.
 */
struct task *alloc_task()
{
    struct task *task = kzalloc(MEM_GEN, sizeof(*task));

    if (!task) {
        errno = -ENOMEM;
        goto error;
    }

    task->as = alloc_address_space();

    if (!task->as) {
        errno = -ENOMEM;
        goto error_task;
    }

    task->pid = next_pid++;

    task->files[0] = open_path("/dev/tty", MODE_READ);
    task->files[1] = open_path("/dev/tty", MODE_WRITE);
    task->files[2] = open_path("/dev/tty", MODE_WRITE);

    list_init(&task->children);
    list_init(&task->children_list);

    return task;

 error_task:
    kfree(task);
 error:
    return NULL;
}

void free_task(struct task *task)
{
    free_address_space(task->as);
    free_kstack(task->esp0);
    kfree(task);
}

/**
 * The idle task's main loop
 */
static void halt()
{
    while (true) {
        asm volatile ("sti\n\t"
                      "hlt\n\t");
    }
}

/**
 * Management functions for the task's kernel stack (each task has its own,
 * switched on context switch)
 */
uint32_t alloc_kstack()
{
    uint32_t stack = (uint32_t)kzalloc(MEM_GEN, PAGE_SIZE);
    return stack + PAGE_SIZE;
}

void free_kstack(uint32_t esp0)
{
    uint32_t stack_bottom = align_down(esp0, PAGE_SIZE);
    kfree((void *)stack_bottom);
}

void switch_tasks(void)
{
    if (!current_task) {
        return;
    }

    struct task *old = current_task;

    if (task_queue_find(&running, old->pid)) {
        task_queue_remove(&running, old);
        task_queue_add(&running, old);
    }

    if (running) {
        current_task = running;
    }
    else {
        current_task = idle;
    }

    if (old == current_task) {
        return;
    }

    switch_address_space(old->as, current_task->as);
    switch_context(current_task);
}

static void timer_handler(registers_t __unused *regs)
{
    static uint32_t ticks = 0;
    ++ticks;

    if (ticks % 5 == 0 && current_task) {
        switch_tasks();
    }
}

/**
 * Set up the task's kernel stack as if we had just pushed data onto it, so
 * we can return from it as if we had just been interrupted when
 * switch_context() is called
 */
unsigned long setup_stack(struct task *task, void *data, unsigned long size)
{
    task->esp0 = alloc_kstack();
    if (!idle->esp0) {
        return -ENOMEM;
    }

    task->esp0 -= size;
    memcpy((void *)task->esp0, data, size);
    return 0;
}

static int create_idle_task(void)
{
    idle = alloc_task();
    if (!idle) {
        return -ENOMEM;
    }

    // Set up the stack as if an interrupt had just occured *right* where
    //   we want the task to begin, so switch_context() can iret smoothly
    // See Intel developer's manual 6.4.1 (page 6-9)
    uint32_t stack[] = {
        0x10, // DS, ES, FS, GS
        0, 0, 0, 0, 0, 0, 0, 0, // general-purpose registers
        0, 0, // interrupt number, error code
        (uint32_t)&halt, // eip
        0x08, // CS
        (1 << 9), // eflags
    };

    int err = setup_stack(idle, stack, sizeof(stack));
    if (err < 0) {
        free_task(idle);
        return err;
    }

    return 0;
}

void init_scheduler(void)
{
    printf("Initializing scheduler...\n");

    int err = create_idle_task();
    if (err < 0) {
        goto error;
    }

    running = alloc_task();
    if (!running) {
        err = -ENOMEM;
        goto error;
    }

    /* blocked = NULL; */
    /* zombies = NULL; */

    printf("allocated idle and init tasks\n");

    current_task = running;
    register_interrupt_handler(32, &timer_handler);
    init_timer(5);

    printf("initialized timer\n");

    err = exec("/init/bin/trash");

    // We shouldn't ever hit this spot - panic if we do
 error:
    printf("[ERROR] Unable to initialize scheduler (%s)\n",
           strerror(-err));
    PANIC();
}

void sleep(void)
{
    task_queue_remove(&running, current_task);
    task_queue_add(&blocked, current_task);

    current_task->status = TASK_BLOCKED;

    asm volatile ("mov $12, %%eax\n\t"
                  "int $0x80\n\t"
                  : : :); // sys_yield() TODO: This shouldn't be syscall 12.. :P
}

void wake(uint32_t pid)
{
    struct task *task = task_queue_find(&blocked, pid);

    if (task) {
        task_queue_remove(&blocked, task);
        task_queue_add(&running, task);

	task->status = TASK_RUNNING;
    }
}
