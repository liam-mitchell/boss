#include "mm.h"

#include "descriptor_tables.h"
#include "errno.h"
#include "interrupt.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
#include "mboot.h"
#include "printf.h"
#include "terminal.h"

#include <stdint.h>

#define for_each_mmap_entry(entry, end, mboot)                                  \
    for (entry = (memory_map_t *)(mboot->mmap_addr),                            \
                       end = entry + mboot_info->mmap_length;                   \
    entry < end;                                                                \
    entry = (memory_map_t *)((unsigned int)entry                                \
                             + entry->size                                      \
                             + sizeof(unsigned long)))

#define MEM_USABLE 1
#define MEM_RESERVED 2
#define MEM_RECLAIMABLE 3
#define MEM_NVS 4
#define MEM_BAD 5

#define PG_PRESENT (1 << 0)
#define PG_WRITEABLE (1 << 1)
#define PG_USER (1 << 2)
#define PG_ACCESSED (1 << 3)
#define PG_DIRTY (1 << 4)

#define PG_IS_PRESENT(p) ((p) & PG_PRESENT)
#define PG_IS_WRITEABLE(p) ((p) & PG_WRITEABLE)
#define PG_IS_USERMODE(p) ((p) & PG_USER)
#define PG_IS_ACCESSED(p) ((p) & PG_ACCESSED)
#define PG_IS_DIRTY(p) ((p) & PG_DIRTY)

extern ldsymbol ld_virtual_offset;

extern ldsymbol ld_page_directory;
extern ldsymbol ld_page_tables;    
extern ldsymbol ld_temp_pages;
extern ldsymbol ld_temp_pages_end;
extern ldsymbol ld_num_temp_pages;
extern ldsymbol ld_virtual_end;
extern ldsymbol ld_physical_end;

static uint32_t free_stack_top;

extern void idt_flush();

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel);
static void unmap_page();

uint32_t **get_page_directory_entry(uint32_t virtual)
{
    return (uint32_t **)0xFFFFF000 + DIRINDEX(virtual);
}

uint32_t *get_page_table(uint32_t virtual)
{
    return (uint32_t *)(0xFFC00000 + DIRINDEX(virtual) * PAGE_SIZE);
}

uint32_t *get_page(uint32_t virtual)
{
    return (uint32_t *)get_page_table(virtual) + TBLINDEX(virtual);
}

uint32_t map_physical(uint32_t physical)
{
    for (uint32_t virtual = (uint32_t)ld_temp_pages;
         virtual < (uint32_t)ld_temp_pages_end;
         virtual += PAGE_SIZE)
    {
        uint32_t *page = get_page(virtual);
        if (!PG_IS_PRESENT(*page)) {
            if (map_page(virtual, physical, 0, 1) < 0) {
                continue;
            }

            printf("Mapping physical address %x to temp pages at %x\n", physical, virtual);
            printf("page: %x at %x\n", *page, (uint32_t)page);
            printf("first byte of page: %d\n", *(char *)virtual);

            uint32_t offset = physical & 0xFFF;
            return virtual + offset;
        }
    }

    printf("failed to map physical address %x", physical);
    return 0;
}

uint32_t alloc_frame()
{
    uint32_t ret = free_stack_top;

    uint32_t virtual = map_physical(ret);
    memcpy(&free_stack_top, (void *)virtual, sizeof(free_stack_top));
    unmap_page(virtual);

    return ret;
}

static void flush_tlb(uint32_t virtual)
{
    asm volatile ("invlpg (%0)" : : "r"(virtual) : );
}

static void unmap_page(uint32_t virtual)
{
    uint32_t **direntry = get_page_directory_entry(virtual);

    if (!PG_IS_PRESENT((uint32_t)*direntry)) {
        PANIC("Attempted to unmap page with no associated page table!");
    }

    uint32_t *page = get_page(virtual);

    if (!PG_IS_PRESENT(*page)) {
        PANIC("Attempted to unmap a page that wasn't there!");
    }

    *page = 0;

    printf("unmapped page at virtual address %x\n", virtual);
    flush_tlb(virtual);
}

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel)
{
    uint32_t **direntry = get_page_directory_entry(virtual);

    if (!PG_IS_PRESENT((uint32_t)*direntry)) {
        uint32_t frame = alloc_frame();
        *direntry =
            (uint32_t *)((frame & 0xFFFFF000)
                         | PG_USER
                         | PG_PRESENT
                         | PG_WRITEABLE);

        puts("Allocated a new directory entry\n");
        if (direntry == NULL) {
            return -ENOMEM;
        }

        memset(get_page_table(virtual), 0, PAGE_SIZE);
    }

    uint32_t *page = get_page(virtual);

    if (PG_IS_PRESENT(*page)) {
        return -EINVAL;
    }

    *page |= PG_PRESENT;

    if (!readonly) {
        *page |= PG_WRITEABLE;
    }

    if (!kernel) {
        *page |= PG_USER;
    }

    *page |= physical & 0xFFFFF000;

    flush_tlb(virtual);
    return 1;
}

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel)
{
    uint32_t physical = alloc_frame();

    if (!physical) {
        return -ENOMEM;
    }

    puts("mapping page...\n");
    map_page(virtual, physical, readonly, kernel);
    puts("mapped page! ");

    uint32_t *page = get_page(virtual);
    puts(PG_IS_PRESENT(*page) ? "present " : "not present ");
    puts(PG_IS_WRITEABLE(*page) ? "writeable " : "not writeable ");
    puts(PG_IS_USERMODE(*page) ? "user " : "not user ");
    puts(" frame: ");
    puth(*page & 0xFFFFF000);
    puts(" virtual: ");
    puth(virtual);
    putc('\n');
    
    return 1;
}

int alloc_pages(uint32_t virtual, uint8_t readonly,
                uint8_t kernel, uint32_t num)
{
    uint32_t start = virtual;
    uint32_t end = virtual + num * PAGE_SIZE;

    int err = 0;
    while (virtual < end) {
        err = alloc_page(virtual, readonly, kernel);
        if (err < 0) {
            goto free_pages;
        }

        virtual += PAGE_SIZE;
    }

    return err;

 free_pages:
    while (virtual > start) {
        virtual -= PAGE_SIZE;
        free_page(virtual);
    }

    return err;
}

void free_page(uint32_t virtual)
{
    memcpy((void *)virtual, &free_stack_top, sizeof(free_stack_top));

    uint32_t *page = get_page(virtual);
    free_stack_top = *page & 0xFFFFF000;

    unmap_page(virtual);
}

static inline int
__attribute__((section(".boot")))
mem_range_is_free(multiboot_info_t *mboot_info,
                  uint32_t start, uint32_t end)
{
    if (mboot_info->flags & (1 << 6)) {
        memory_map_t *entry;
        memory_map_t *last_entry;

        for_each_mmap_entry(entry, last_entry, mboot_info) {
            uint32_t entry_start = entry->base_addr_low;
            uint32_t entry_end = entry_start + entry->length_low;

            if (entry_end < start) {
                continue;
            }

            if (entry_start < start) {
                if (entry_end > end) {
                    return 1;
                }
                else {
                    start = entry_end;
                }
            }
            if (start > end) {
                return 0;
            }
        }
    }

    return 0;
}

/**
 * Initialize the free frame stack by embedding a pointer to the next
 * free frame in the first word of of each free frame
 *
 * This requires the virtual memory manager to return us each free frame pointer
 * once it maps the frame into virtual memory - however, it requires only four
 * bytes of memory for the physical memory manager total!
 */
uint32_t
__attribute__((section (".text")))
init_free_frame_stack(multiboot_info_t *mboot_info)
{
    uint32_t *top = &free_stack_top;
    top -= ((uint32_t)ld_virtual_offset / sizeof(uint32_t));

    *top = 0;

    uint32_t address;
    uint32_t count = 0;

    if (!(mboot_info->flags & (1 << 6))) {
        return 0;
    }

    for_each_frame(address) {
        if (address < (uint32_t)ld_physical_end) {
            continue;
        }

        /* Push the address onto the free frame stack */
        if (mem_range_is_free(mboot_info, address, address + PAGE_SIZE)) {
            *(uint32_t *)address = *top;
            *top = address;
            ++count;
        }
    }

    return count;
}

static void page_fault_handler(registers_t *registers)
{
    uint32_t address;
    asm volatile ("mov %%cr2, %0" : "=r"(address) : : );

    printf("[PAGE FAULT] address: %x (%s %s %s page)\n", address,
           (registers->error & 0x4) ? "user" : "kernel",
           (registers->error & 0x2) ? "write to" : "read from",
           (registers->error & 0x1) ? "present" : "not present");
}

void init_paging()
{
    register_interrupt_callback(0x0E, &page_fault_handler);
    flush_idt();

    /* uint32_t virtual; */
    /* for_each_frame(virtual) { */
    /*     uint32_t *page = get_page(virtual); */
    /*     uint32_t *pgtbl = (uint32_t*)get_page_directory_entry(virtual); */
    /*     if (PG_IS_PRESENT(*pgtbl) && PG_IS_PRESENT(*page)) { */
    /*         printf("page at %x: page ptr %x\n", virtual, page); */
    /*         printf("page: %x pgtbl %x\n", *page, *pgtbl); */
    /*     } */
    /*     else { */
    /*         //            printf("pgtbl not present: %x\n", *pgtbl); */
    /*     } */
    /* } */
    
    memcpy(&kdirectory, ld_page_directory, sizeof(kdirectory));
}
