        [BITS 32]

hello:  
        mov eax, 0x4
        mov ebx, 0x1
        mov ecx, msg
        mov edx, len
        int 0x80

fork:   
        mov eax, 2
        int 0x80

exec:   
        cmp eax, 0
        jg  goodbye
        mov eax, 0xB
        mov ebx, path
        int 0x80

goodbye:        
        mov eax, 0x4
        mov ebx, 0x1
        mov ecx, forkmsg
        mov edx, forklen
        int 0x80
        jmp $
        
        section .data
msg:	db "Hello usermode world!", 0xA, 0
len:	equ $ - msg

path:	db "/init/bin/goodbye", 0

forkmsg:db "Forked off a child, now this process is done", 0xA, 0
forklen:equ $ - forkmsg
