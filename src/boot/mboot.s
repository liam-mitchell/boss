        ;; mboot.s
        ;; multiboot header definitions
	MBOOT_ALIGN           equ 1<<0
	MBOOT_MEMINFO         equ 1<<1
	MBOOT_MAGIC           equ 0x1BADB002 
	MBOOT_FLAGS           equ MBOOT_ALIGN | MBOOT_MEMINFO
	MBOOT_CHECKSUM        equ -(MBOOT_MAGIC + MBOOT_FLAGS)

SECTION .boot
ALIGN 0x4
        dd MBOOT_MAGIC
        dd MBOOT_FLAGS
        dd MBOOT_CHECKSUM
