#include "errno.h"
#include "interrupt.h"
#include "kheap.h"
#include "macros.h"
#include "mm.h"
#include "memory.h"
#include "mboot.h"
#include "terminal.h"
#include "descriptor_tables.h"

#include <stdint.h>

#define for_each_frame(address)                                                 \
    for (address = 0;                                                           \
         address < 0xFFFFF000;                                                  \
         address += PAGE_SIZE)

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
#define PG_WRITABLE (1 << 1)
#define PG_USER (1 << 2)
#define PG_ACCESSED (1 << 3)
#define PG_DIRTY (1 << 4)

extern uint32_t KERNEL_VIRTUAL_OFFSET;
extern uint32_t KERNEL_VIRTUAL_START;

extern uint32_t kernel_page_directory;
extern uint32_t kernel_page_tables;    
extern uint32_t kernel_virtual_end;
extern uint32_t kernel_physical_end;

static page_directory_t *currdirectory =
    (page_directory_t *)&kernel_page_directory;

static uint32_t free_stack_top;

extern void idt_flush();

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel);
static void unmap_page();

static page_table_t **get_page_directory_entry(uint32_t virtual)
{
    return (page_table_t **)(0xFFFFF000) + DIRINDEX(virtual);
}

static page_table_t *get_page_table(uint32_t virtual)
{
    return (page_table_t *)(0xFFC00000 + DIRINDEX(virtual) * PAGE_SIZE);
}

static page_t *get_page(uint32_t virtual)
{
    return (page_t *)get_page_table(virtual) + TBLINDEX(virtual);
}

uint32_t map_physical(uint32_t physical)
{
    for (uint32_t virtual = (uint32_t)&KERNEL_VIRTUAL_START;
         virtual < 0xFFFFF000;
         virtual += PAGE_SIZE)
    {
        page_t *page = get_page(virtual);
        if (!page->present) {
            map_page(virtual, physical, 0, 1);
            uint32_t offset = physical & 0xFFF;
            return virtual + offset;
        }
    }

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
    page_table_t **direntry = get_page_directory_entry(virtual);

    if (!((uint32_t)(*direntry) & PG_PRESENT)) {
        PANIC("Attempted to unmap page with no associated page table!");
    }

    page_t *page = get_page(virtual);

    if (!page->present) {
        PANIC("Attempted to unmap a page that wasn't there!");
    }
    
    page->present = 0;
    flush_tlb(virtual);
}

/* static void read_next_page(uint32_t virtual) */
/* { */
/*     puts("reading page @"); */
/*     puth(virtual); */
/*     putc('\n'); */
/*     memcpy(&free_stack_top, (void *)virtual, sizeof(free_stack_top)); */
/* } */

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel)
{
    page_table_t **direntry = get_page_directory_entry(virtual);
    
    if (!((uint32_t)(*direntry) & PG_PRESENT)) {
        *direntry =
            (page_table_t *)((alloc_frame() & 0xFFFFF000)
                             | PG_USER
                             | PG_PRESENT
                             | PG_WRITABLE);

        puts("Allocated a new directory entry\n");
        if (direntry == NULL) {
            return -ENOMEM;
        }
    }

    page_t *page = get_page(virtual);

    if (page->present) {
        return -EINVAL;
    }

    page->present = 1;
    page->writeable = !readonly;
    page->user = !kernel;
    page->frame = physical >> 12;

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

    page_t *page = get_page(virtual);
    puts(page->present ? "present " : "not present ");
    puts(page->writeable ? "writeable " : "not writeable ");
    puts(page->user ? "user " : "not user ");
    puts(" frame: ");
    puth(page->frame);
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

    page_t *page = get_page(virtual);
    free_stack_top = (page->frame) << 12;
    
    unmap_page(virtual);
}

static void clone_page_table(page_table_t *dest, const page_table_t *src)
{
    memset(dest, 0, sizeof(*dest));

    for (uint32_t i = 0; i < PAGE_SIZE / 4 - 1; ++i) {
        uint32_t physical = alloc_frame();
        uint32_t frame = physical >> 12;

        memcpy(&dest->pages[i], &src->pages[i], sizeof(page_t));
        dest->pages[i].frame = frame;
    }
}

void clone_address_space(address_space_t *dest, const address_space_t *src)
{
    asm volatile("cli");

    /* puts("cloning address space @"); */
    /* puth((uint32_t)src); */
    /* puts("->"); */
    /* puth((uint32_t)dest); */
    /* putc('\n'); */

    /* puts("dest pgdir @"); */
    /* puth((uint32_t)dest->pgdir); */
    /* putc('\n'); */
    
    memset(dest->pgdir, 0, sizeof(*(dest->pgdir)));

    /* puts("zeroed memory...\n"); */

    dest->prog_break = src->prog_break;
    dest->stack_top = src->stack_top;

    for (uint32_t i = 0; i < PAGE_SIZE / 4; ++i) {
        page_table_t *ktable = kdirectory.tables[i];
        page_table_t *srctable = src->pgdir->tables[i];

        if (ktable == srctable) {
            /* puts("linking page table: "); */
            /* puth(i); */
            /* puts(" @"); */
            /* puth((uint32_t)src->pgdir->tables[i]); */
            /* putc('\n'); */
            dest->pgdir->tables[i] = srctable;
        }
        else if (srctable) {
            /* puts("copying page table: "); */
            /* puth(i); */
            /* puts(" @"); */
            /* puth((uint32_t)src->pgdir->tables[i]); */
            /* putc('\n'); */

            dest->pgdir->tables[i] = kmalloc(PAGE_SIZE);
            clone_page_table(dest->pgdir->tables[i],
                             src->pgdir->tables[i]);
        }
    }

    puts("cloned address space!\n");

    asm volatile("sti");
}

void switch_address_space(address_space_t *old, const address_space_t *new)
{
    page_directory_t *kdir = &kdirectory;

    for (uint32_t dirindex = 0; dirindex < PAGE_SIZE / 4; ++dirindex) {
        page_table_t *ktable = kdir->tables[dirindex];
        page_table_t *currtable = currdirectory->tables[dirindex];
        old->pgdir->tables[dirindex] = currtable;
        
        if (!ktable || (ktable == currtable)) {
            /* puts("skipping table "); */
            /* puth(dirindex); */
            /* puts(" @"); */
            /* puth((uint32_t)ktable); */
            /* puts(ktable ? " (kernel)" : " (null)"); */
            /* putc('\n'); */
            continue;
        }

        /* puts("copying table "); */
        /* puth(dirindex); */
        /* puts(" @"); */
        /* puth((uint32_t)ktable); */
        /* putc('\n'); */

        currdirectory->tables[dirindex] = new->pgdir->tables[dirindex];

        for (uint32_t tblindex = 0; tblindex < PAGE_SIZE / 4; ++tblindex) {
            flush_tlb((dirindex << 22) & (tblindex << 12));
        }
    }
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
    top -= ((uint32_t)&KERNEL_VIRTUAL_OFFSET / sizeof(uint32_t));

    *top = 0;

    uint32_t address;
    uint32_t count = 0;

    if (!(mboot_info->flags & (1 << 6))) {
        return 0;
    }

    for_each_frame(address) {
        if (address < (uint32_t)&kernel_physical_end) {
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

uint32_t top() {return free_stack_top;}
uint32_t top_addr() {return (uint32_t)&free_stack_top;}

static void page_fault_handler(registers_t *registers)
{
    uint32_t address;
    asm volatile ("mov %%cr2, %0" : "=r"(address) : : );

    puts("[PAGE FAULT] ");
    puts("address: ");
    puth(address);
    puts(" (");
    puts(registers->error & 0x1 ? "not present " : "");
    puts(registers->error & 0x2 ? "writing " : "");
    puts(registers->error & 0x4 ? "user " : "kernel ");
    puts(registers->error & 0x8 ? "reserved " : "");
    puts(")\n");
}

void init_paging()
{
    register_interrupt_callback(0x0E, &page_fault_handler);
    flush_idt();

    memcpy(&kdirectory, &kernel_page_directory, sizeof(kdirectory));
}
