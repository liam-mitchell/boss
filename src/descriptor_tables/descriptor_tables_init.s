        ;; descriptor_tables_init.s - startup code for interrupt and global descriptor tables
        ;; author: Liam Mitchell

	[GLOBAL idt_flush]
        [GLOBAL gdt_flush]
        
idt_flush:
	mov eax, [esp + 4]
	lidt [eax]
	ret

gdt_flush:
	mov eax, [esp + 4]
	lgdt [eax]

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:flush
flush:
	ret
