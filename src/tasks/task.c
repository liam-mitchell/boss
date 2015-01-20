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

/* static void print_tasks() */
/* { */
/*     printf("idle task %d\n", idle->pid); */
/*     print_regs(&idle->regs, "idle task regs"); */
/*     printf("current task %d\n", current_task->pid); */
/*     print_regs(&current_task->regs, "current task regs"); */

/*     for (task_t *r = running; r; r = r->next) { */
/*         printf("running task %d\n", r->pid); */
/*         print_regs(&r->regs, "regs"); */
/*     } */

/*     for (task_t *b = blocked; b; b = b->next) { */
/*         printf("blocked task %d\n", b->pid); */
/*         print_regs(&b->regs, "regs"); */
/*     } */
/* } */

extern void save_context(registers_t *old);
extern void restore_context(registers_t *new);

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
        return NULL;
    }

    task->as = alloc_address_space();

    if (!task->as) {
        errno = -ENOMEM;
        kfree(task);
        return NULL;
    }

    task->pid = next_pid++;

    task->files[0] = open_path("/dev/tty", MODE_READ);
    task->files[1] = open_path("/dev/tty", MODE_WRITE);
    task->files[2] = open_path("/dev/tty", MODE_WRITE);

    return task;
}

/* static void halt() */
/* { */
/*     asm volatile ("sti\n\t" */
/*                   "hlt\n\t"); */
/* } */

/* static void create_idle_task() */
/* { */
/*     task_t *task = alloc_task(); */
/*     void *stack = kzalloc(MEM_GEN, PAGE_SIZE); */

/*     task->regs.eip = (uint32_t)&halt; */
/*     task->regs.esp = (uint32_t)stack + PAGE_SIZE - 4; */
/*     task->regs.ss = 0x10; */
/*     task->regs.cs = 0x08; */

/*     idle = task; */
/* } */

static void free_task(task_t *task)
{
    free_address_space(task->as);
    kfree(task);
}

/* static void kthread_jump(task_t *task) */
/* { */
/*     asm volatile (".intel_syntax noprefix\n\t" */
/*                   "cli\n\t" */
/* } */

static void usermode_jump(task_t *task)
{
    /* print_regs(&task->regs, "jumping to usermode"); */
    /* printf("first instruction word: %x\n", *(uint32_t *)task->regs.eip); */
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

    /* PANIC("usermode jump failed?!"); */
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


static void switch_tasks(/* registers_t *regs */)
{
    if (!current_task) {
        return;
    }

    /* printf("switching tasks\n"); */
    task_t *old = current_task;

    if (task_list_find(&running, old->pid)) {
        /* printf("requeueing task %d\n", old->pid); */
        task_list_remove(&running, old);
        task_list_add(&running, old);
    }
    else {
        /* printf("task %d not found to requeue, must be sleeping\n", old->pid); */
    }

    if (running) {
        /* printf("switching to running task %d\n", running->pid); */
        /* print_regs(&idle->regs, "idle task registers"); */
        current_task = running;
    }
    else {
        current_task = idle;

        if (old != current_task) {
            /* printf("switching to idle task\n"); */
            /* print_tasks(); */
            switch_address_space(old->as, current_task->as);
            usermode_jump(current_task);
        }
    }

    if (old == current_task) {
        /* printf("not switching at all, we were already here\n"); */
        return;
    }

    /* print_tasks(); */

    /* *regs = current_task->regs; */
    /* printf("switching tasks from %d to %d\n", old->pid, current_task->pid); */
    switch_address_space(old->as, current_task->as);
    restore_context(&current_task->regs);
}

static void timer_handler(registers_t *regs)
{
    static uint32_t ticks = 0;
    ++ticks;

    if (ticks % 5 == 0 && current_task) {
        current_task->regs = *regs;
        switch_tasks();
    }
}

static int as_alloc_page(address_space_t *as, uint32_t virtual,
                         bool readonly, bool kernel)
{
    uint32_t *pde = (uint32_t *)&as->pgdir[DIRINDEX(virtual)];
    if (!PG_IS_PRESENT(*pde)) {
        uint32_t frame = alloc_frame();
        if (!frame) {
            return -ENOMEM;
        }

        *pde = frame | PG_USER | PG_WRITEABLE | PG_PRESENT;
    }

    uint32_t *pt = (uint32_t *)(map_physical(*pde) & ~0xFFF);
    uint32_t *page = &pt[TBLINDEX(virtual)];

    *page = alloc_frame();
    if (!*page) {
        free_frame(PG_FRAME(*pde));
        return -ENOMEM;
    }

    set_page_attributes(page, true, !readonly, !kernel);

    unmap_page((uint32_t)pt);

    return 0;
}

static void as_free_page(address_space_t *as, uint32_t virtual)
{
    /* TODO: free the page tables too if we can... */
    uint32_t *pde = (uint32_t *)&as->pgdir[DIRINDEX(virtual)];
    if (!PG_IS_PRESENT(*pde)) {
        return;
    }

    uint32_t *pt = (uint32_t *)map_physical(PG_FRAME(*pde));
    uint32_t *page = &pt[TBLINDEX(virtual)];

    if (*page) {
        free_frame(PG_FRAME(*page));
    }

    unmap_page((uint32_t)pt);
}

static int map_data(address_space_t *as, uint32_t len, void *data)
{
    void *start = data;
    int err = 0;
    for (uint32_t virtual = 0; virtual < len; virtual += PAGE_SIZE) {
        err = as_alloc_page(as, virtual, false, false);
        if (err < 0) {
            goto free_pages;
        }

        uint32_t *pt =
            (uint32_t *)map_physical(PG_FRAME((uint32_t)as->pgdir[DIRINDEX(virtual)]));
        uint32_t *page =
            (uint32_t *)map_physical(PG_FRAME(pt[TBLINDEX(virtual)]));

        memcpy(page, data, min(len - virtual, (uint32_t)PAGE_SIZE));
        data += PAGE_SIZE;

        unmap_page((uint32_t)page);
        unmap_page((uint32_t)pt);
    }

    as->brk = align(len, PAGE_SIZE);
    return 0;

 free_pages:
    for (uint32_t virtual = data - start;
         virtual <= (uint32_t)data;
         virtual -= PAGE_SIZE)
    {
        as_free_page(as, virtual);
    }

    return err;
}

static int map_stack(address_space_t *as)
{
    return as_alloc_page(as, (uint32_t)ld_virtual_offset - 4, false, false);
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
    /* int err = 0; */
    /* file_t *binary = open_path("/init/bin/idle", MODE_READ); */
    /* if (!binary) { */
    /*     err = -ENOENT; */
    /*     goto error; */
    /* } */

    /* void *data = kmalloc(MEM_GEN, binary->length); */
    /* if (!data) { */
    /*     err = -ENOMEM; */
    /*     goto error_file; */
    /* } */

    /* uint32_t len = vfs_read(binary, &binary->offset, binary->length, data); */
    /* if (len < binary->length) { */
    /*     err = -EIO; */
    /*     goto error_filedata; */
    /* } */

    /* if (current_task->as) { */
    /*     free_address_space(current_task->as); */
    /* } */

    printf("allocating address space\n");

    int err = 0;
    idle = alloc_task();
    if (!idle) {
        err = -ENOMEM;
        goto error;
    }

    char data[2] = { '\xeb', '\xfe' };
    printf("allocated as, mapping data %x%x", data[0], data[1]);

    err = map_data(idle->as, sizeof(data), data);
    if (err < 0) {
        goto error_task;
    }

    printf("mapped data, mapping stack\n");
    err = map_stack(idle->as);
    if (err < 0) {
        goto error_task;
    }

    printf("mapped stack\n");

    init_exec_registers(&idle->regs);
    /* print_regs(&idle->regs, "allocated idle registers"); */
    return err;

 error_task:
    free_task(idle);
    
 error:
    return err;
}

void init_scheduler(void)
{
    printf("Initializing scheduler...\n");

    asm volatile ("cli");
    int err = create_idle_task();
    if (err < 0) {
        goto error;
    }

    printf("Created idle task\n");
    printf("idle task eip: %x\n", idle->regs.eip);
    /* print_regs(&idle->regs, "idle task regs 0"); */

    running = alloc_task();
    if (!running) {
        err = -ENOMEM;
        goto error;
    }

    printf("allocated init task\n");
    /* print_regs(&idle->regs, "idle task regs 1"); */

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

    if (current_task->as) {
        free_address_space(current_task->as);
    }

    current_task->as = alloc_address_space();
    if (!current_task->as) {
        err = -ENOMEM;
        goto error_filedata;
    }

    err = map_data(current_task->as, len, data);
    if (err < 0) {
        goto error_as;
    }

    err = map_stack(current_task->as);
    if (err < 0) {
        goto error_as;
    }

    init_exec_registers(&current_task->regs);
    switch_address_space(NULL, current_task->as);

    /* print_regs(&idle->regs, "idle task regs"); */
    print_regs(&current_task->regs, "task 1 regs");

    usermode_jump(current_task);

 error_as:
    free_address_space(current_task->as);

 error_filedata:
    kfree(data);

 error_file:
    vfs_close(binary);

 error:
    return err;
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

    /* child->state = TASK_RUNNING; */

    child->regs = current_task->regs;
    child->regs.eax = 0;
    task_list_add(&running, child);
    printf("Forking task %d into %d\n", current_task->pid, child->pid);
    return child->pid;

 error:
    free_task(child);
    return err;
}

void sleep(void)
{
    task_list_remove(&running, current_task);
    task_list_add(&blocked, current_task);

    /* save_context(&current_task->regs); */
    /* printf("task %d sleeping... eip %x\n", current_task->pid, current_task->regs.eip); */
    switch_tasks(); // TODO: this doesn't save context...

    /* while (true) { */
    /*     if (running) { */
    /*         usermode_jump(running); */
    /*     } */

    /*     asm volatile ("sti\n\t" */
    /*                   "hlt\n\t"); */
    /* } */
}

void wake(uint32_t pid)
{
    /* printf("waking task with pid %x\n", pid); */
    struct task *task = task_list_find(&blocked, pid);

    if (task) {
        /* printf("found task\n"); */
        task_list_remove(&blocked, task);
        task_list_add(&running, task);
    }
}
