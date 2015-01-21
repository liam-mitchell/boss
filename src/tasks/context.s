        [GLOBAL save_context]
        [GLOBAL restore_context]

save_context:
;;         push ss
;;         push esp
;;         pushf
;;         push cs
;;         call .save

;; .save:
;;         pusha
;; 	mov ax, ds
;; 	push eax

	;; mov ax, 0x10
	;; mov ds, ax
	;; mov es, ax
	;; mov fs, ax
	;; mov gs, ax
        ;; mov [esp + 4], ds
        ;; mov [esp + 8], edi
        ;; mov [esp + 12], esi
        ;; mov [esp + 16], ebp
        ;; mov [esp + 20], 0
        ;; mov [esp + 24], ebx
        ;; mov [esp + 28], edx
        ;; mov [esp + 32], ecx
        ;; mov [esp + 36], eax
        ;; mov [esp + 40], 0
        ;; mov [esp + 44], 0
        ;; mov [esp + 48], 

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
