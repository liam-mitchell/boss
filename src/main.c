// main.c - entry point for kernel from the bootloader
// author: Liam Mitchell

#include "descriptor_tables.h"
#include "fork.h"
#include "fs.h"
#include "kheap.h"
#include "macros.h"
#include "mboot.h"
#include "memory.h"
#include "mm.h"
#include "modules.h"
#include "task.h"
#include "terminal.h"
#include "timer.h"
#include "vfs.h"

#ifdef __cplusplus
EXTERN
#endif // __cplusplus

/**
 * kernel's main function
 * called from boot.s
 */

extern uint32_t KERNEL_VIRTUAL_START;
extern uint32_t kernel_virtual_end;

void kernel_main(multiboot_info_t *mboot, uint32_t magic)
{
    terminal_init();
    puts("Hello, world of OS development!\n");

    if (magic != 0x2BADB002) {
        puth(magic);
        puts(" ");
        puth((uint32_t)mboot);
        puts(" ");
        PANIC(" Incorrect magic number from grub!");
    }

    if (mboot->mods_count == 0) {
        PANIC("No modules loaded!\n");
    }

    puts("mboot->flags: ");
    puth(mboot->flags);
    putc('\n');
    puts("mboot->mods_addr: ");
    puth(mboot->mods_addr);
    putc('\n');
    /* puts("module->mod_start: "); */
    /* puth(((module_t*)mboot->mods_addr)->mod_start); */
    /* puts("module->mod_end: "); */
    /* puth(((module_t*)mboot->mods_addr)->mod_end); */
    
    init_descriptor_tables();
    init_paging();

    asm volatile("sti");
    timer_init(50);

    init_kheap();

    init_modules(mboot);

    /* file_t *placeholder = */
    /*     open_path("/init/bin/placeholder.txt", MODE_READ | MODE_WRITE); */
    /* puts("placeholder->ino: "); */
    /* puti(placeholder->inode->ino); */
    /* putc('\n'); */

    /* void *buf = kmalloc(placeholder->length); */
    /* uint32_t len = vfs_read(placeholder, &placeholder->offset, placeholder->length, buf); */

    /* puts("read placeholder.txt: "); */
    /* puts("(offset "); */
    /* puth(placeholder->offset); */
    /* puts(" length "); */
    /* puth(placeholder->length); */
    /* puts(")\ndata: "); */
    /* puts((char *)buf); */
    /* putc('\n'); */
    /* puts("read len: "); */
    /* puth(len); */
    /* putc('\n'); */

    /* file_t *hello = */
    /*     open_path("/init/hello.txt", MODE_READ | MODE_WRITE); */

    /* if (!hello) { */
    /*     PANIC("Couldn't find hello.txt!"); */
    /* } */
    
    /* kfree(buf); */
    /* buf = kmalloc(hello->length); */
    /* len = vfs_read(hello, &hello->offset, hello->length, buf); */

    /* puts("read hello.txt: "); */
    /* puts("(offset "); */
    /* puth(hello->offset); */
    /* puts(" length "); */
    /* puth(hello->length); */
    /* puts(")\ndata: "); */
    /* puts((char *)buf); */
    /* putc('\n'); */
    /* puts("read len: "); */
    /* puth(len); */
    /* putc('\n'); */
}

#ifdef __cplusplus
ENDEXTERN
#endif // __cplusplus
