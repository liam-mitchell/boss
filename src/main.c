// main.c - entry point for kernel from the bootloader
// author: Liam Mitchell

#include "descriptor_tables.h"
#include "fs.h"
#include "keyboard.h"
#include "kheap.h"
#include "macros.h"
#include "mboot.h"
#include "memory.h"
#include "pmm.h"
#include "pci.h"
#include "printf.h"
#include "syscalls.h"
#include "task.h"
#include "terminal.h"
#include "timer.h"
#include "vfs.h"
#include "vmm.h"

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

    if (magic != 0x2BADB002) {
        PANIC("Incorrect magic number from grub!");
    }

    if (mboot->mods_count == 0) {
        PANIC("No modules loaded!\n");
    }

    /* puts("module->mod_start: "); */
    /* puth(((module_t*)mboot->mods_addr)->mod_start); */
    /* puts("module->mod_end: "); */
    /* puth(((module_t*)mboot->mods_addr)->mod_end); */
    
    init_descriptor_tables();
    init_paging();

    asm volatile("sti");
    timer_init(50);

    init_kheap();
    init_filesystem();
    init_keyboard();
    init_syscalls();
    /* init_pci(); */
    init_scheduler();
    
    /* file_t *placeholder = */
    /*     open_path("/init/bin/placeholder.txt", MODE_READ | MODE_WRITE); */
    /* puts("placeholder->ino: "); */
    /* puti(placeholder->inode->ino); */
    /* putc('\n'); */

    /* void *buf = kmalloc(placeholder->length); */
    /* uint32_t len = vfs_read(placeholder, &placeholder->offset, placeholder->length, buf); */

    /* printf("read placeholder.txt:\n" */
    /*        " offset: %x\n" */
    /*        " length: %d\n" */
    /*        " data: %s\n" */
    /*        "\n read length: %d\n", */
    /*        placeholder->offset, */
    /*        placeholder->length, */
    /*        (char *)buf, */
    /*        len); */

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

    /* file_t *test = open_path("/init/bin/test.s", MODE_READ); */

    /* char *buf = kmalloc(test->length); */
    /* uint32_t len = vfs_read(test, &test->offset, test->length, buf); */

    /* printf("data from test:\n"); */
    /* for (uint32_t i = 0; i < len; ++i) { */
    /*     printf("%c", buf[i]); */
    /* } */
    /* printf("end test file\n"); */
}

#ifdef __cplusplus
ENDEXTERN
#endif // __cplusplus
