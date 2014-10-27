	;; boot.s - kernel entry point
        ;; author: Liam Mitchell
[BITS 32]

        extern KERNEL_BOOT_OFFSET
        extern KERNEL_VIRTUAL_OFFSET
        extern KERNEL_VIRTUAL_START
        extern KERNEL_PAGE_NUMBER
        extern kernel_physical_end
        extern kernel_page_directory
        extern kernel_page_tables
        extern kernel_page_table_low
        extern kernel_num_tables
        extern kernel_main
        
section .bootstrap_stack
align 4096
        times 4096 dd 0
kernel_stack:   

section .text
global _start

_start:
        mov esp, kernel_stack
        sub esp, KERNEL_VIRTUAL_OFFSET
        
        push eax                ; Push multiboot magic number
        push ebx                ; and pointer to multiboot struct

        mov eax, kernel_page_directory
        sub eax, KERNEL_VIRTUAL_OFFSET
        
        mov ebx, kernel_page_table_low
        sub ebx, KERNEL_VIRTUAL_OFFSET

        or  ebx, 0x3        
        mov [eax], ebx          ; kernel_page_directory[0] =
                                ;         (kernel_page_table_low & ~0x3FF) | 0x3
        
        ;; Recursive page directory trick - map the directory to itself so we can
        ;; access the page mappings later

        mov ebx, eax
        or  ebx, 0x3
        mov [eax + 4092], ebx
        
        ;; Identity map the first first 4MB of memory for now.
        ;; We can unmap this later once we have virtual page tables set up too

        ;; eax = address
        ;; ebx = kernel_page_tables
        ;; ecx = counter
        mov ebx, kernel_page_table_low
        sub ebx, KERNEL_VIRTUAL_OFFSET
        
        xor ecx, ecx                    ; for (int address = 0;
        xor eax, eax
physical_map:                           ; address < 0x00100000
        cmp ecx, 1024                   ; address += 0x1000) {
        jge physical_map_end
        
        ;; edx = address >> 12 (page table index)
        mov edx, eax
        shr edx, 12
        and edx, 0x3FF
        
        ;; eax = address | 0x3
        or  eax, 0x3

        ;; kernel_page_table[address >> 12] = address | 0x3
        mov [ebx + edx * 4], eax
        
        ;; eax = address
        and eax, 0xFFFFF000

        inc ecx
        add eax, 0x1000                 ; address += 0x1000
        jmp physical_map
physical_map_end:
        ;; now we need to fill the page directory with entries for our
        ;; predefined kernel page tables

        ;; int count = 0;
        ;; while (count < kernel_num_tables) {
        ;;     kernel_page_directory[count + KERNEL_PAGE_NUMBER] =
        ;;                      (kernel_page_tables + i << 10) | 0x7
        ;;     ++count;
        ;; }

        mov eax, kernel_page_directory
        sub eax, KERNEL_VIRTUAL_OFFSET
        
        xor ecx, ecx                    ; ecx = count
directory_map:
        cmp ecx, kernel_num_tables
        jge directory_map_end

        ;; ebx = count + KERNEL_PAGE_NUMBER
        mov ebx, ecx                    ; ebx = count
        add ebx, KERNEL_PAGE_NUMBER     ; ebx = count + KERNEL_PAGE_NUMBER
        
        ;; edx = (kernel_page_tables + (count << 10)) | 0x7        
        mov edx, ecx                    ; edx = count
        shl edx, 10                     ; edx = count << 10
        add edx, kernel_page_tables     ; edx = (count << 10) + kernel_page_tables
        sub edx, KERNEL_VIRTUAL_OFFSET  ; still physical pointers
        or  edx, 0x7                    ; edx = ((count << 10) + kernel_page_tables) | 0x7

        ;; kernel_page_directory[count + KERNEL_PAGE_NUMBER] = edx
        mov [eax + ebx * 4], edx

        inc ecx                         ; ++count
        jmp directory_map
directory_map_end:
        
        ;; now, map the kernel into the top 1G

        ;; uint32_t virtual = KERNEL_VIRTUAL_START & 0xFFFFF000
        ;; uint32_t physical = KERNEL_BOOT_OFFSET & 0xFFFFF000

        ;; while (physical < kernel_physical_end) {
        ;;     kernel_page_directory[virtual >> 22]->[virtual >> 12 & 0x3FF]
        ;;                                               = physical | 0x3
        ;;     virtual += 4096
        ;;     physical += 4096
        ;; }
        mov eax, KERNEL_VIRTUAL_START   ; eax = virtual 
        mov ecx, KERNEL_BOOT_OFFSET     ; ecx = physical
        
virtual_map:
        cmp ecx, kernel_physical_end
        jge virtual_map_end

        ;; kernel_page_directory[virtual >> 22]->[virtual >> 12 & 0x3FF] = physical | 0x3
        ;; ebx = directory index
        ;; edx = table index
        mov ebx, eax
        shr ebx, 22

        mov edx, eax
        shr edx, 12
        and edx, 0x3FF

        mov esi, kernel_page_directory     ; esi = kernel_page_directory
        sub esi, KERNEL_VIRTUAL_OFFSET     ; 
        
        mov edi, [esi + 4 * ebx]           ; edi = kernel_page_directory[dirindex]
        and edi, 0xFFFFF000                ; remove information bits
        
        mov ebx, ecx
        or  ebx, 0x3
        mov [edi + 4 * edx], ebx           ; kernel_page_directory[dirindex][tblindex] =
                                           ;                            physical | 0x3
        add eax, 0x1000                    ; virtual += 4096
        add ecx, 0x1000                    ; physical += 4096
        jmp virtual_map
        
virtual_map_end:
        ;; Tell the processor where our page directory is
        mov eax, kernel_page_directory
        ;; Still have to use physical addresses for this
        sub eax, KERNEL_VIRTUAL_OFFSET
        mov cr3, eax

        ;; Enable paging
        mov ebx, cr0
        or  ebx, 0x80000000
        mov cr0, ebx

        ;; Jump to the higher half kernel
        lea eax, [higherhalf]
        jmp eax
higherhalf:
	call kernel_main
end:
	jmp end
