#include "task.h"

#include "algorithm.h"
#include "bool.h"
#include "descriptor_tables.h"
#include "errno.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "memory.h"
#include "pmm.h"
#include "port.h"
#include "printf.h"
#include "timer.h"
#include "vfs.h"
#include "vmm.h"

extern ldsymbol ld_virtual_offset;

static uint32_t next_pid;

static struct task *running;
static struct task *blocked;
static struct task *idle;

static void task_list_add(struct task **list, struct task *task)
{
    if (!task) {
        return;
    }
    
    struct task *curr = *list;

    if (!curr) {
        *list = task;
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

static void task_list_remove(struct task **list, struct task *task)
{
    if (!task) {
        return;
    }

    if (*list == task) {
        *list = task->next;
    }

    if (task->prev) {
        task->prev->next = task->next;
    }

    if (task->next) {
        task->next->prev = task->prev;
    }
}

static task_t *task_list_find(struct task **list, uint32_t pid)
{
    for (task_t *task = *list; task; task = task->next) {
        if (task->pid == pid) {
            return task;
        }
    }

    return NULL;
}

static task_t *alloc_task()
{
    task_t *task = kzalloc(MEM_GEN, sizeof(*task));

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

    return task;

 error_task:
    kfree(task);
 error:
    return NULL;
}

static void halt()
{
    while (true) {
        asm volatile ("sti\n\t"
                      "hlt\n\t");
    }
}

static uint32_t alloc_kstack()
{
    uint32_t stack = (uint32_t)kzalloc(MEM_GEN, PAGE_SIZE);
    return stack + PAGE_SIZE;
}

static void free_kstack(uint32_t esp0)
{
    uint32_t stack_bottom = align_down(esp0, PAGE_SIZE);
    kfree((void *)stack_bottom);
}

static void free_task(task_t *task)
{
    free_address_space(task->as);
    free_kstack(task->esp0);
    kfree(task);
}

static void usermode_jump(task_t *task)
{
    asm volatile (".intel_syntax noprefix\n\t"
                  "cli\n\t"
                  "mov ax, 0x23\n\t"
                  "mov ds, ax\n\t"
                  "mov es, ax\n\t"
                  "mov fs, ax\n\t"
                  "mov gs, ax\n\t"
                  "push %0\n\t"
                  "push %1\n\t"

                  "pushf\n\t"
                  "pop eax\n\t"
                  "or eax, 0x200\n\t"
                  "push eax\n\t"

                  "push %2\n\t"
                  "push %3\n\t"
                  "iret\n\t"
                  ".att_syntax noprefix\n\t"
                  :
                  : "r"(task->regs.ss), "r"(task->regs.esp),
                    "r"(task->regs.cs), "r"(task->regs.eip)
                  : "eax");
}

void print_regs(registers_t *regs, char *msg)
{
    printf("%s:\n"
           " eip: %x\n"
           " esp: %x\n"
           " cs:  %x\n"
           " ss:  %x\n"
           " eax: %x\n"
           " ecx: %x\n"
           " edx: %x\n"
           " ebp: %x\n"
           " esi: %x\n"
           " edi: %x\n",
           msg,
           regs->eip,
           regs->esp,
           regs->cs,
           regs->ss,
           regs->eax,
           regs->ecx,
           regs->edx,
           regs->ebx,
           regs->esi,
           regs->edi);
}

#define switch_context(old, new) do {                                   \
    set_esp0(new->esp0);                                                \
    uint32_t eax;                                                       \
    asm volatile ("movl %1, %%esp\n\t"                                  \
                  "pop %%eax\n\t"                                       \
                  "mov %%ax, %%ds\n\t"                                  \
                  "mov %%ax, %%es\n\t"                                  \
                  "mov %%ax, %%fs\n\t"                                  \
                  "mov %%ax, %%gs\n\t"                                  \
                  "popa\n\t"                                            \
                  "add $8, %%esp\n\t"                                   \
                  "iret\n\t"                                            \
                  : "=a"(eax)                                           \
                  : "g"(new->esp0)                                      \
                  : "%ebx", "%ecx", "%edx", "%esi", "%edi");            \
    } while (0);

void switch_tasks(void)
{
    if (!current_task) {
        return;
    }

    task_t *old = current_task;

    if (task_list_find(&running, old->pid)) {
        task_list_remove(&running, old);
        task_list_add(&running, old);
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
    switch_context(old, current_task);
}

static void timer_handler(registers_t __unused *regs)
{
    static uint32_t ticks = 0;
    ++ticks;

    if (ticks % 5 == 0 && current_task) {
        switch_tasks();
    }
}

static void init_exec_registers(registers_t *regs)
{
    regs->esp = (uint32_t)ld_virtual_offset - 4;
    regs->eip = 0;
    regs->cs = 0x18 | 0x3;
    regs->ss = 0x20 | 0x3;
    regs->eflags = 1 << 9;
}

static int create_idle_task(void)
{
    idle = alloc_task();
    if (!idle) {
        return -ENOMEM;
    }

    idle->esp0 = alloc_kstack();
    if (!idle->esp0) {
        kfree(idle);
        return -ENOMEM;
    }

    uint32_t stack[] = {
        0x10,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0,
        (uint32_t)&halt,
        0x08,
        (1 << 9),
    };

    for (uint32_t i = ARRAY_SIZE(stack); i > 0; --i) {
        idle->esp0 -= sizeof(stack[i - 1]);
        memcpy((void *)idle->esp0, &stack[i - 1], sizeof(stack[i - 1]));
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

    printf("allocated idle and init tasks\n");

    current_task = running;
    register_interrupt_callback(32, &timer_handler);
    init_timer(5);

    printf("initialized timer\n");
    
    err = exec("/init/bin/trash");
    if (err < 0) {
        goto error;
    }

    return;

 error:
    printf("[ERROR] Unable to initialize scheduler (%s)\n",
           strerror(-err));
    PANIC();
}

int exec(const char *path)
{
    int err = 0;
    file_t *binary = open_path(path, MODE_READ);
    if (!binary) {
        err = -ENOENT;
        goto error;
    }

    void *data = kmalloc(MEM_GEN, binary->length);
    if (!data) {
        err = -ENOMEM;
        goto error_file;
    }

    uint32_t len = vfs_read(binary, &binary->offset, binary->length, data);
    if (len < binary->length) {
        err = -EIO;
        goto error_filedata;
    }

    free_address_space(current_task->as);
    current_task->as = alloc_address_space();
    if (!current_task->as) {
        err = -ENOMEM;
        goto error_filedata;
    }

    /* free_kstack(current_task->esp0); FIXME: this doesn't work... :( */
    current_task->esp0 = alloc_kstack();
    if (!current_task->esp0) {
        goto error_as;
    }

    err = map_as_data(current_task->as, len, data);
    if (err < 0) {
        goto error_kstack;
    }

    err = map_as_stack(current_task->as);
    if (err < 0) {
        goto error_kstack;
    }

    init_exec_registers(&current_task->regs);
    switch_address_space(NULL, current_task->as);
    usermode_jump(current_task);

 error_kstack:
    free_kstack(current_task->esp0);

 error_as:
    free_address_space(current_task->as);

 error_filedata:
    kfree(data);

 error_file:
    vfs_close(binary);

 error:
    return err;
}

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
    asm volatile ("cli");

    int err = 0;

    task_t *child = alloc_task();
    if (!child) {
        err = -ENOMEM;
        goto error;
    }

    child->as = clone_address_space();
    if (!child->as) {
        err = -ENOMEM;
        goto error;
    }

    child->regs = current_task->regs;
    child->regs.eax = 0;

    child->esp0 = clone_kstack(current_task->esp0);
    if (!child->esp0) {
        err = -ENOMEM;
        goto error;
    }

    registers_t *child_regs = (registers_t *)child->esp0;
    child_regs->eax = 0;

    task_list_add(&running, child);

    return child->pid;

 error:
    free_task(child);
    return err;
}

void sleep(void)
{
    task_list_remove(&running, current_task);
    task_list_add(&blocked, current_task);

    asm volatile ("mov $12, %%eax\n\t"
                  "int $0x80\n\t"
                  : : :); // sys_yield() TODO: This shouldn't be syscall 12... :P
}

void wake(uint32_t pid)
{
    struct task *task = task_list_find(&blocked, pid);

    if (task) {
        task_list_remove(&blocked, task);
        task_list_add(&running, task);
    }
}
