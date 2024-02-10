// main.c - entry point for kernel from the bootloader
// author: Liam Mitchell

#include "macros.h"
#include "mboot.h"
#include "syscalls.h"
#include "task.h"
#include "test.h"

#include "device/descriptor_tables.h"
#include "device/keyboard.h"
#include "device/terminal.h"
#include "fs/fs.h"
#include "memory/kheap.h"
#include "memory/vmm.h"

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

    ktest();

    init_scheduler();
}
