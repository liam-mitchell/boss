        [BITS 32]

        ;; mov eax, 0x2
        ;; int 0x80
        ;; cmp eax, 0
        ;; jg writeprompt

        ;; mov eax, 0xB
        ;; mov ebx, goodbye
        ;; int 0x80
        
writeprompt:
        mov eax, 0x4            ; sys_write
        mov ebx, 0x1            ; stdout
        mov ecx, prompt         ; buffer
        mov edx, prlen          ; length
        int 0x80

getline:
        mov esi, 0              ; esi = len

getchar:
        mov eax, 0x3            ; sys_read
        mov ebx, 0x0            ; stdin
        mov ecx, buffer         ; buffer
        add ecx, esi            ; buffer + len
        mov edx, 1              ; read one character
        int 0x80

        ;; cmp eax, 0x20
        ;; je  getchar

        mov eax, 0x4            ; sys_write
        mov ebx, 0x1            ; stdout
        mov edx, 1
        int 0x80
        
        mov eax, [buffer + esi]
        cmp eax, 0xA
        je  getline_end

        inc esi
        cmp esi, 79
        jge getline_end

        jmp getchar

getline_end:
        mov [buffer + esi], dword 0x0
        push esi

        mov eax, 0x2            ; sys_fork
        int 0x80

        cmp eax, 0
        jg  writeprompt         ; jump back to prompt to exec again

        mov eax, 0xB            ; or exec in this if we're the child
        mov ebx, buffer
        int 0x80

error:
        pop esi
        mov eax, 0x4            ; sys_write
        mov ebx, 0x1            ; stdout
        mov ecx, err            ; error message - we came back from exec
        mov edx, errlen         ; length
        int 0x80

        mov eax, 0x4            ; sys_write
        mov ebx, 0x1            ; stdout
        mov ecx, buffer         ; buffer
        mov edx, esi            ; length
        int 0x80

        mov eax, 0x4
        mov ebx, 0x1
        mov ecx, nl
        mov edx, 1
        int 0x80

        jmp $                   ; should be exit() really

        section .data

buffer: resb 80

prompt: db "[trash] $ ", 0
prlen:  equ $ - prompt

err:    db "[trash] error: exec failed: ", 0
errlen: equ $ - err

goodbye:db "/init/bin/goodbye", 0

nl:     db 0xA
