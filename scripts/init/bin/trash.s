        [BITS 32]

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
        je  exec                ; if we're the child, exec

        mov ebx, eax            ; ebx = returned child pid
        mov eax, 0x7            ; sys_wait
        mov ecx, status         ; ecx = (int*)status
        int 0x80                ; sys_wait(pid, status)

        mov eax, [status]
        add eax, 48             ; convert status to ASCII
        mov [statusmsg + statuslen - 3], al ; add status to the end of statusmsg (before newline and null terminator)

        mov eax, 0x4            ; sys_write
        mov ebx, 0x1            ; stdout
        mov ecx, statusmsg      ; buffer
        mov edx, statuslen      ; length
        int 0x80                ; sys_write(stdout, statusmsg, statuslen)
        jmp writeprompt

exec:
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

nl:     db 0xA

status: dd 0x0

statusmsg:
        db "[trash] process returned  ", 10, 0

statuslen: equ $ - statusmsg
