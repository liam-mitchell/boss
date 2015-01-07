        [GLOBAL outb]
        [GLOBAL outw]
        [GLOBAL outl]
        [GLOBAL inb]
        [GLOBAL inw]
        [GLOBAL inl]
        [SECTION .text]

outb:
        ;; edx = port
        ;; eax = byte
        xor eax, eax
        xor edx, edx
        mov dx, word [esp + 4]
        mov al, byte [esp + 8]
        out dx, al
        ret

outw:
        xor eax, eax
        xor edx, edx
        mov dx, word [esp + 4]
        mov ax, word [esp + 8]
        out dx, ax
        ret

outl:
        xor eax, eax
        xor edx, edx
        mov dx, word [esp + 4]
        mov eax, [esp + 8]
        out dx, eax
        ret

inb:
        ;; dx = port
        xor eax, eax
        xor edx, edx
        mov dx, word [esp + 4]
        in al, dx
        ret

inw:
        xor eax, eax
        xor edx, edx
        mov dx, word [esp + 4]
        in ax, dx
        ret

inl:
        xor eax, eax
        xor edx, edx
        mov dx, word [esp + 4]
        in eax, dx
        ret
