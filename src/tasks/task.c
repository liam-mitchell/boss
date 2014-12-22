#include "task.h"

#include "kheap.h"
#include "macros.h"
#include "memory.h"
#include "mm.h"

#define TASK_STACK_SIZE 0x1000
#define TASK_STACK_TOP 0xC0000000

extern uint32_t kernel_stack;
extern uint32_t kernel_page_directory;
static uint32_t next_pid = 1;

extern uint32_t read_eip();

/* static void move_stack(task_t *task) */
/* { */
/*     puts("moving stack...\n"); */
/*     uint32_t stack = TASK_STACK_TOP; */
/*     uint32_t stack_bot = stack - TASK_STACK_SIZE; */

/*     puts("allocating page @"); */
/*     puth(stack_bot); */
/*     putc('\n'); */

/*     int err = alloc_page(stack_bot, 0, 0); */
/*     if (err < 0) { */
/*         PANIC("Unable to create new process stack!"); */
/*     } */

/*     puts("allocated page for new stack...\n"); */
/*     puts("old stack at: "); */
/*     puth((uint32_t)&kernel_stack); */
/*     putc('\n'); */

/*     uint32_t old_stack_bot; */
/*     asm volatile("mov %%esp, %0" : "=r"(old_stack_bot) : : ); */
/*     uint32_t old_stack_size = (uint32_t)&kernel_stack - old_stack_bot; */
/*     task->as.esp = stack - old_stack_size; */

/*     puts("new esp: "); */
/*     puth(task->as.esp); */
/*     putc('\n'); */

/*     puts("stack top: "); */
/*     puth(stack); */
/*     putc('\n'); */

/*     puts("old stack bot: "); */
/*     puth(old_stack_bot); */
/*     puts("\n\n\n\n"); */

/*     puts("copying stack...\n"); */

/*     memcpy((void *)stack_bot, */
/*            (void *)(old_stack_bot & 0xFFFFF000), */
/*            PAGE_SIZE); */

/*     asm volatile("mov %0, %%esp" : : "r"(task->as.esp) : ); */
/* } */

task_t *alloc_task()
{
    task_t *task = kzalloc(sizeof(*task));
    task->pid = next_pid++;
    return task;
}

void schedule_task(task_t *task)
{
    puts("scheduling task...\n");
    puts(" eip: ");
    puth(task->regs.eip);
    puts(" ebp: ");
    puth(task->regs.ebp);
    puts(" esp: ");
    puth(task->as.esp);
    putc('\n');
    
    task_t *current = current_task;

    if (current == NULL) {
        current_task = task;
        current_task->next = NULL;
        puts("scheduled first task:");
        puts(" eip: ");
        puth(current_task->regs.eip);
        puts(" ebp: ");
        puth(current_task->regs.ebp);
        puts(" esp: ");
        puth(current_task->as.esp);
        putc('\n');
        return;
    }
    
    while (current->next != NULL) {
        current = current->next;
    }
    
    current->next = task;
    task->next = NULL;

    puts("scheduled task: ");
    puts(" eip: ");
    puth(task->regs.eip);
    puts(" ebp: ");
    puth(task->regs.ebp);
    puts(" esp: ");
    puth(task->as.esp);
    putc('\n');
}

static void copy_kernel_int_regs(registers_t *dest, const registers_t *src)
{
    dest->ds = src->ds;
    dest->edi = src->edi;
    dest->esi = src->esi;
    dest->ebp = src->ebp;
    dest->unused = src->unused;
    dest->ebx = src->ebx;
    dest->edx = src->edx;
    dest->ecx = src->ecx;
    dest->eax = src->eax;
    dest->interrupt = src->interrupt;
    dest->error = src->error;
    dest->eip = src->eip;
    dest->cs = src->cs;
    dest->eflags = src->eflags;
}

void switch_tasks(registers_t *registers)
{
    asm volatile("cli");

    if (current_task == NULL || current_task->next == NULL) {
        puts("current task was null!\n");
        return;
    }

    asm volatile ("mov %%esp, %0" : "=r"(current_task->as.esp) : : );

    puts("switch task called with esp: ");
    puth(current_task->as.esp);
    putc('\n');

    /* puts("first word on task stack: "); */
    /* puth(*(uint32_t *)registers->esp); */
    /* putc('\n'); */

    task_t *old = current_task;
    current_task = current_task->next;
    schedule_task(old);
    
    if (old->as.pgdir != &kdirectory) {
        PANIC("Can't switch to user tasks yet!");
    }
    else {
        copy_kernel_int_regs(&old->regs, registers);
        copy_kernel_int_regs(registers, &current_task->regs);
        puts("switching task: from: ");
        puts(" eip: ");
        puth(old->regs.eip);
        puts(" ebp: ");
        puth(old->regs.ebp);
        puts(" esp: ");
        puth(old->as.esp);
        putc('\n');
        puts("switching task: to: ");
        puts(" eip: ");
        puth(registers->eip);
        puts(" ebp: ");
        puth(registers->ebp);
        puts(" esp: ");
        puth(current_task->as.esp);
        putc('\n');
        asm volatile ("mov %0, %%esp" : : "r"(current_task->as.esp) : );
    }

    /* task_t *task = current_task; */
    /* current_task = current_task->next; */

    /* if (current_task) { */
    /*     *registers = current_task->regs; */
    /*     switch_address_space(&current_task->as); */
    /* } */

    /* putc('\n'); */
    /* putc('\n');     */

    asm volatile("sti");
}

void init_scheduler()
{
    task_t *task = alloc_task();
    puts("allocating a task @");
    puth((uint32_t)task);
    putc('\n');

    task->as.pgdir = &kdirectory;
    task->as.prog_break = 0;
    task->as.stack_top = (uint32_t)&kernel_stack;

    asm volatile ("mov %%esp, %0" : "=r"(task->as.esp) : : );

    schedule_task(task);

    puts("initial task: ");
    puts(" task esp: ");
    puth(task->as.esp);
    puts(" ebp: ");
    puth(task->regs.ebp);
    puts(" eip: ");
    puth(task->regs.eip);
    putc('\n');
}
