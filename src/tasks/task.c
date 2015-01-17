#include "task.h"

#include "algorithm.h"
#include "bool.h"
#include "descriptor_tables.h"
#include "errno.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "memory.h"
#include "pmm.h"
#include "printf.h"
#include "vfs.h"
#include "vmm.h"

extern ldsymbol ld_virtual_offset;

extern void (*enter_usermode)(task_t *task);

static uint32_t next_pid;

static task_t *alloc_task()
{
    task_t *task = kzalloc(MEM_GEN, sizeof(*task));
    task->as = alloc_address_space();
    task->pid = next_pid++;
    return task;
}

static void free_task(task_t *task)
{
    free_address_space(task->as);
    kfree(task);
}

static void task_enqueue(task_t *task)
{
    if (!run_queue) {
        run_queue = task;
        return;
    }

    task_t *curr = run_queue;
    while (curr->next) {
        curr = curr->next;
    }

    curr->next = task;
    task->next = NULL;
}

static task_t *task_dequeue()
{
    if (!run_queue) {
        return NULL;
    }

    task_t *ret = run_queue;
    run_queue = run_queue->next;
    return ret;
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

static void switch_tasks(registers_t *regs)
{
    if (!run_queue) {
        return;
    }

    task_t *old = task_dequeue();
    old->regs = *regs;

    task_enqueue(old);

    /* print_regs(&run_queue->regs, "switching tasks - new:"); */
    *regs = run_queue->regs;
    switch_address_space(old->as, run_queue->as);
}

static void timer_handler(registers_t *regs)
{
    static uint32_t ticks = 0;
    ++ticks;

    /* printf("recieved timer interrupt\n"); */

    if (ticks % 100 == 0) {
        switch_tasks(regs);
    }
}

void init_scheduler()
{
    printf("Initializing scheduler...\n");
    run_queue = alloc_task();
    register_interrupt_callback(32, &timer_handler);

    /* asm volatile ("sti"); */
    int err = exec("/init/bin/test");

    if (err < 0) {
        printf("[ERROR] Unable to initialize scheduler (%s)\n",
               strerror(-err));
        PANIC();
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

void init_exec_registers(registers_t *regs)
{
    regs->esp = (uint32_t)ld_virtual_offset - 4;
    regs->eip = 0;
    regs->cs = 0x18 | 0x3;
    regs->ss = 0x20 | 0x3;
    regs->eflags = 1 << 9;
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

    if (run_queue->as) {
        free_address_space(run_queue->as);
    }

    run_queue->as = alloc_address_space();
    if (!run_queue->as) {
        err = -ENOMEM;
        goto error_filedata;
    }

    err = map_data(run_queue->as, len, data);
    if (err < 0) {
        goto error_as;
    }

    err = map_stack(run_queue->as);
    if (err < 0) {
        goto error_as;
    }

    init_exec_registers(&run_queue->regs);
    switch_address_space(NULL, run_queue->as);
    usermode_jump(run_queue);

 error_as:
    free_address_space(run_queue->as);

 error_filedata:
    kfree(data);

 error_file:
    vfs_close(binary);

 error:
    return err;
}

int fork()
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

    child->state = TASK_RUNNING;
    child->regs = run_queue->regs;
    child->regs.eax = 0;
    task_enqueue(child);
    printf("Forking task %d into %d\n", run_queue->pid, child->pid);
    return child->pid;

 error:
    free_task(child);
    return err;
}
