// main.c - entry point for kernel from the bootloader
// author: Liam Mitchell

#include "descriptor_tables.h"
#include "fs.h"
#include "keyboard.h"
#include "kheap.h"
#include "macros.h"
#include "mboot.h"
#include "syscalls.h"
#include "task.h"
#include "terminal.h"
#include "vmm.h"

/**
 * kernel's main function
 * called from boot.s
 */

void kernel_main(multiboot_info_t *mboot, uint32_t magic)
{
    init_terminal();

    if (magic != 0x2BADB002) {
        PANIC("Incorrect magic number from grub!");
    }

    if (mboot->mods_count == 0) {
        PANIC("No modules loaded!\n");
    }

    init_descriptor_tables();
    init_paging();
    init_kheap();
    init_filesystem();
    init_keyboard();
    init_syscalls();
    /* init_pci(); */
    init_scheduler();
}
