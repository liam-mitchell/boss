// main.c - entry point for kernel from the bootloader
// author: Liam Mitchell

#include "terminal.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "kheap.h"
#include "mboot.h"
#include "macros.h"
#include "mm.h"

#ifdef __cplusplus
EXTERN
#endif // __cplusplus

/**
 * kernel's main function
 * called from boot.s
 */

extern uint32_t kernel_virtual_end;

void kernel_main()
{
    terminal_init();
    puts("Hello, world of OS development!\n");

    init_descriptor_tables();

    asm volatile("sti");
    timer_init(50);

    puts("kernel_virtual_end: ");
    puth((uint32_t)&kernel_virtual_end);
    putc('\n');

    puts("free_stack_top: ");
    puth(top());
    putc('\n');

    puts("&free_stack_top: ");
    puth(top_addr());
    putc('\n');
    
    init_kheap();

    uint32_t *allocated = kmalloc(sizeof(*allocated));
    *allocated = 42;
    uint32_t *a2 = kmalloc(sizeof(*a2));
    puts("allocated an integer: ");
    puti(*allocated);
    putc('\n');

    puts("allocated: ");
    puth((uint32_t)allocated);
    putc('\n');
    
    puts("a2: ");
    puth((uint32_t)a2);
    putc('\n');

    kfree(allocated);
    allocated = kmalloc(sizeof(*allocated));
    
    puts("allocated: ");
    puth((uint32_t)allocated);
    putc('\n');
}

#ifdef __cplusplus
ENDEXTERN
#endif // __cplusplus
