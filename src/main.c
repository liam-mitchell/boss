// main.c - entry point for kernel from the bootloader
// author: Liam Mitchell

#include "terminal.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "mem-detect.h"
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

void kernel_main()
{
    /* terminal_init(); */
    /* puts("Hello, world of OS development!\n"); */

    /* init_descriptor_tables(); */
    /* /\* init_bios_mem_map(mboot_info); *\/ */
    /* /\* print_bios_mem_map(); *\/ */

    /* puts("kheap_top: "); */
    /* puth(top()); */
    /* putc('\n'); */

    /* /\* init_paging(); *\/ */

    /* asm volatile("sti"); */
    /* timer_init(50); */
}

#ifdef __cplusplus
ENDEXTERN
#endif // __cplusplus
