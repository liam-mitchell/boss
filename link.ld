ENTRY(_start)

ld_boot_offset = 1M;
ld_virtual_offset = 0xC0000000; /* 3GB */
ld_virtual_start = ld_boot_offset + ld_virtual_offset;
ld_first_kernel_page = ld_virtual_offset >> 22;
ld_num_temp_pages = 4;

SECTIONS
{
	.boot ld_boot_offset : AT(ld_boot_offset)
        {
                ld_phys_start = .;
                *(.boot)
                . = ALIGN(4K);
                ld_boot_end = .;
        }

        .text ld_boot_end + ld_virtual_offset : AT(ld_boot_end)
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
        ld_page_directory = .;
        . += 4K;

        . = ALIGN(4K);
        ld_page_tables = .;
        ld_num_tables = (((0xFFFFFFFF - (ld_virtual_start & 0xFFC00000)) + 1) / 0x400000) - 1;
        . += ld_num_tables * 4K;
        
        . = ALIGN(4K);
        ld_tss_stack = .;
        . += 4K;

        . = ALIGN(4K);
        ld_initrd = .;
        . += 4K;

        . = ALIGN(4K);
        ld_physical_end = . - ld_virtual_offset;
        ld_virtual_end = .;

        . = ALIGN(4K);
        ld_screen = .;
        . += 4K;

        . = ALIGN(4K);
        ld_temp_pages = .;
        . += ld_num_temp_pages * 4K;
        ld_temp_pages_end = .;

        . = ALIGN(4K);
        ld_page_table_low = .;
        . += 4K;

        . = ALIGN(4K);
        ld_heap_start = .;
        . += 4K;
}
