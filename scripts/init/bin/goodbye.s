        [BITS 32]
        mov eax, 0x4
        mov ebx, 0x1
        mov ecx, msg
        mov edx, len
        int 0x80

	mov eax, 0x1
	mov ebx, 0x1
	int 0x80

        section .data
msg:    db "Goodbye usermode world!", 0xA, 0
len:    equ $ - msg
