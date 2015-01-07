#include "task.h"

#include "algorithm.h"
#include "bool.h"
#include "descriptor_tables.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "memory.h"
#include "mm.h"
#include "printf.h"
#include "vfs.h"

extern ldsymbol ld_virtual_offset;

extern void (*enter_usermode)(task_t *task);

static uint32_t next_pid;

static task_t *alloc_task()
{
    task_t *task = kzalloc(sizeof(*task));
    task->as = alloc_address_space();
    task->pid = next_pid++;
    return task;
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

static void switch_address_space(task_t *task)
{
    asm volatile("cli");

    for (uint32_t virtual = 0;
         virtual < (uint32_t)ld_virtual_offset;
         virtual += PAGE_SIZE)
    {
        uint32_t *pde = (uint32_t *)get_page_directory_entry(virtual);
        uint32_t new_pde = (uint32_t)task->as->pgdir[DIRINDEX(virtual)];

        if (PG_FRAME(*pde) != PG_FRAME(new_pde)) {
            *pde = new_pde;
            flush_tlb(virtual);
        }

        if (PG_IS_PRESENT(*pde)) {
            uint32_t *new_pt = (uint32_t *)map_physical(new_pde & ~0xFFF);
            if (!new_pt) {
                PANIC("unable to map new page table");
            }

            uint32_t *pg = get_page(virtual);
            uint32_t new_pg = new_pt[TBLINDEX(virtual)];


            if (PG_FRAME(*pg) != PG_FRAME(new_pg)) {
                *pg = new_pg;
                flush_tlb(virtual);
            }

            unmap_page((uint32_t)new_pt);
        }
    }

    asm volatile("sti");
}

static void print_regs(registers_t *regs, char *msg)
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
    print_regs(regs, "switching tasks - old:");
    old->regs = *regs;

    task_enqueue(old);

    print_regs(&run_queue->regs, "switching tasks - new:");
    *regs = run_queue->regs;
    switch_address_space(run_queue);
}

static void timer_handler(registers_t *regs)
{
    static uint32_t ticks = 0;
    ++ticks;

    if (ticks % 100 == 0) {
        printf("tick: %d\n", ticks);
        switch_tasks(regs);
    }
}

void init_scheduler()
{
    printf("Initializing scheduler...\n");
    run_queue = alloc_task();
    register_interrupt_callback(32, &timer_handler);
    exec("/init/bin/test");
}

static void as_alloc_page(address_space_t *as, uint32_t virtual,
                          bool readonly, bool kernel)
{
    uint32_t *pde = (uint32_t *)&as->pgdir[DIRINDEX(virtual)];
    if (!PG_IS_PRESENT(*pde)) {
        *pde = alloc_frame() | PG_USER | PG_PRESENT | PG_WRITEABLE;
    }

    uint32_t *pt = (uint32_t *)(map_physical(*pde) & ~0xFFF);
    uint32_t *page = &pt[TBLINDEX(virtual)];

    *page = alloc_frame();
    set_page_attributes(page, true, !readonly, !kernel);

    unmap_page((uint32_t)pt);
}

static void map_data(address_space_t *as, uint32_t len, void *data)
{
    for (uint32_t virtual = 0; virtual < len; virtual += PAGE_SIZE) {
        as_alloc_page(as, virtual, false, false);

        uint32_t *pt =
            (uint32_t *)map_physical(PG_FRAME((uint32_t)as->pgdir[DIRINDEX(virtual)]));
        uint32_t *page =
            (uint32_t *)map_physical(PG_FRAME(pt[TBLINDEX(virtual)]));

        memcpy(page, data, min(len - virtual, (uint32_t)PAGE_SIZE));
        data += PAGE_SIZE;

        unmap_page((uint32_t)page);
        unmap_page((uint32_t)pt);
    }
}

static void map_stack(address_space_t *as)
{
    as_alloc_page(as, (uint32_t)ld_virtual_offset - 4, false, false);
}

static void usermode_jump(task_t *task)
{
    printf("usermode jumping to eip %x\n"
           "first word: %x\n"
           "second word: %x\n",
           task->regs.eip, *(uint32_t *)task->regs.eip, *(uint32_t *)(task->regs.eip + 4));
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
                  /* "pop eax\n\t" */
                  /* "pop ebx\n\t" */
                  /* "popf\n\t" */
                  /* "pop ecx\n\t" */
                  /* "pop edx\n\t" */
                  /* "jmp $\n\t" */
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

void exec(const char *path)
{
    file_t *binary = open_path(path, MODE_READ);
    void *data = kmalloc(binary->length);
    uint32_t len = vfs_read(binary, &binary->offset, binary->length, data);

    if (len < binary->length) {
        printf("Unable to read all file data from %s for exec (%d/%d bytes read)",
               path, len, binary->length);
        return;
    }

    run_queue->as = alloc_address_space();
    map_data(run_queue->as, len, data);
    map_stack(run_queue->as);

    init_exec_registers(&run_queue->regs);

    switch_address_space(run_queue);
    usermode_jump(run_queue);
}
