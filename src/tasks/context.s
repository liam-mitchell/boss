        [GLOBAL save_context]
        [GLOBAL restore_context]

save_context:
        ;; push ss
        ;; push esp
        ;; pushf
        ;; push cs
        ;; push eip

        ;; pusha
	;; mov ax, ds
	;; push eax

	;; mov ax, 0x10
	;; mov ds, ax
	;; mov es, ax
	;; mov fs, ax
	;; mov gs, ax


restore_context:
        ;; mov esp, [esp + 4]      ; esp + 4 = new context
        ;; ret
        ;; jmp $
        mov esp, [esp + 4]

        pop eax
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

        popa
        add esp, 8
        iret
