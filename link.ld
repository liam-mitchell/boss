ENTRY(_start)

KERNEL_BOOT_OFFSET = 1M;
KERNEL_VIRTUAL_OFFSET = 0xC0000000; /* 3GB */
KERNEL_VIRTUAL_START = KERNEL_BOOT_OFFSET + KERNEL_VIRTUAL_OFFSET;
KERNEL_PAGE_NUMBER = KERNEL_VIRTUAL_OFFSET >> 22;

SECTIONS
{
	.boot KERNEL_BOOT_OFFSET : AT(KERNEL_BOOT_OFFSET)
        {
                kernel_phys_start = .;
                *(.boot)
                . = ALIGN(4K);
                kernel_boot_end = .;
        }

        .text kernel_boot_end + KERNEL_VIRTUAL_OFFSET : AT(kernel_boot_end)
        {
		*(.multiboot)	
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
        
        . = ALIGN(4K);
        kernel_page_directory = .;
        . += 4K;

        . = ALIGN(4K);
        kernel_page_tables = .;
        kernel_num_tables = (((0xFFFFFFFF - (KERNEL_VIRTUAL_START & 0xFFC00000)) + 1) / 0x400000) - 1;
        . += kernel_num_tables * 4K;
        
        . = ALIGN(4K);
        tss_stack = .;
        . += 4K;

        . = ALIGN(4K);
        kernel_screen = .;
        . += 4K;

        . = ALIGN(4K);
        kernel_page_table_low = .;
        . += 4K;

        . = ALIGN(4K);
        kernel_physical_end = . - KERNEL_VIRTUAL_OFFSET;
        kernel_virtual_end = .;

        . = ALIGN(4K);
        kernel_heap_start = .;
        . += 4K;
}
