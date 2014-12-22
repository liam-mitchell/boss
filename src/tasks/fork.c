#include "fork.h"

#include "kheap.h"
#include "memory.h"
#include "mm.h"
#include "task.h"
#include "terminal.h"

#define PROC_STACK_SIZE 0x4000

extern uint32_t read_eip();

extern uint32_t kernel_temp_table_src;
extern uint32_t kernel_temp_table_dest;

/* static void clone_stack(task_t *dest, const task_t *src) */
/* { */
/*     puts("cloning stack into task @"); */
/*     puth((uint32_t)dest); */
/*     puts(" from task @"); */
/*     puth((uint32_t)src); */
/*     putc('\n'); */

/*     uint32_t esp_dir = DIRINDEX(src->regs.esp); */
/*     uint32_t esp_tbl = TBLINDEX(src->regs.esp); */

/*     uint32_t new_stack = alloc_frame(); */

/*     puts("allocated frame: "); */
/*     puth(new_stack); */
/*     putc('\n'); */
    
/*     page_t *src_stack_page = */
/*         &src->as.pgdir->tables[esp_dir]->pages[esp_tbl]; */
/*     page_t *dest_stack_page = */
/*         &dest->as.pgdir->tables[esp_dir]->pages[esp_tbl]; */

/*     puts("src stack page: "); */
/*     puth((uint32_t)src_stack_page); */
/*     puts(" dest stack page: "); */
/*     puth((uint32_t)dest_stack_page); */
/*     putc('\n'); */

/*     map_temp_pages(new_stack, (src_stack_page->frame << 12)); */

/*     memcpy((void *)&kernel_temp_table_dest, */
/*            (void *)&kernel_temp_table_src, */
/*            PAGE_SIZE); */

/*     unmap_temp_pages(); */

/*     dest_stack_page->frame = (new_stack >> 12); */
/* } */

static uint32_t clone_kstack(uint32_t stack)
{
    uint32_t new = (uint32_t)kmalloc(PAGE_SIZE);
    memcpy((void *)new, (void *)stack, PAGE_SIZE);
    return new + PAGE_SIZE;
}

int fork()
{
    asm volatile("cli");

    task_t *task = current_task;
    task_t *new = alloc_task();

    if (task->as.pgdir == &kdirectory) {
        puts("forking kernel task...\n");
        new->as.pgdir = &kdirectory;
        new->as.prog_break = task->as.prog_break;
        new->as.stack_top = clone_kstack(task->as.stack_top);
        puts("cloning kernel stack @");
        puth(task->as.stack_top);
        puts("->");
        puth(new->as.stack_top);
        putc('\n');
    }
    else {
        new->as.pgdir = kmalloc(PAGE_SIZE);
        clone_address_space(&new->as, &task->as);
    }

    new->parent_pid = task->pid;
    new->regs = task->regs;

    uint32_t stack_size = task->as.stack_top - task->as.esp;
    new->as.esp = new->as.stack_top - stack_size;

    schedule_task(new);

    new->regs.eip = read_eip();

    puts("forked eip: ");
    puth(new->regs.eip);
    putc('\n');

    if (task == current_task) {
        asm volatile("sti");
        return new->pid;
    }
    else {
        return 0;
    }
}
