#include "errno.h"
#include "macros.h"
#include "mm.h"
#include "memory.h"
#include "mboot.h"
#include "terminal.h"

#include <stdint.h>

#define DIRINDEX(virtual) ((virtual) >> 22)
#define TBLINDEX(virtual) (((virtual) >> 12) & 0x3FF)

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

extern uint32_t kernel_page_directory;
extern uint32_t kernel_page_tables;    
extern uint32_t kernel_virtual_end;
extern uint32_t kernel_physical_end;

static uint32_t free_stack_top;

typedef struct page {
    uint32_t present : 1;
    uint32_t writeable : 1;
    uint32_t user : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t unused : 7;
    uint32_t frame : 20;
} page_t;

typedef struct page_table {
    page_t pages[1024];
} page_table_t;

typedef struct page_directory {
    page_table_t *tables[1024];
} page_directory_t;

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

static uint32_t alloc_frame()
{
    return free_stack_top;
}

static void unmap_page(uint32_t virtual)
{
    page_table_t **direntry = get_page_directory_entry(virtual);

    if (!((uint32_t)(*direntry) & PG_PRESENT)) {
        PANIC("Attempted to unmap page with no associated page table!");
    }

    page_t *page = get_page(virtual);

    if (!page->present) {
        PANIC("Attempted to unmap page that wasn't mapped!");
    }
    
    page->present = 0;
}

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel);

static void read_next_page(uint32_t virtual)
{

    page_table_t **direntry = get_page_directory_entry(virtual);

    page_t *de = (page_t *)direntry;

    page_t *pt = (page_t *)get_page_table(virtual);

    puts("table: ");
    puth((uint32_t)pt);
    putc('\n');

    puts("TBLINDEX(virtual): ");
    puth(TBLINDEX(virtual));
    putc('\n');

    puts("DIRINDEX(virtual): ");
    puth(DIRINDEX(virtual));
    putc('\n');
    
    puts("direntry: ");
    puth((uint32_t)direntry);
    putc('\n');

    puts("*direntry: ");
    puth((uint32_t)*direntry);
    putc('\n');

    puts("direntry->writeable: ");
    puth(de->writeable);
    putc('\n');

    puts("direntry->user: ");
    puth(de->user);
    putc('\n');

    puts("direntry->frame: ");
    puth(de->frame);
    putc('\n');

    puts("direntry->present: ");
    puth(de->present);
    putc('\n');

    puts("read_next_page(): virtual: ");
    puth(virtual);
    putc('\n');

    puts("free_stack_top: ");
    puth(free_stack_top);
    putc('\n');

    puts("&free_stack_top: ");
    puth((uint32_t)&free_stack_top);
    putc('\n');

    puts("get_page(virtual)->present: ");
    puth(get_page(virtual)->present);
    putc('\n');

    puts("get_page(virtual)->frame: ");
    puth(get_page(virtual)->frame);
    putc('\n');

    puts("get_page(virtual)->writeable: ");
    puth(get_page(virtual)->writeable);
    putc('\n');

    puts("get_page(virtual)->user: ");
    puth(get_page(virtual)->user);
    putc('\n');

    /* puts("*virtual: "); */
    /* puth(*(uint32_t *)virtual); */
    /* putc('\n'); */

    memcpy(&free_stack_top, (void *)virtual, sizeof(free_stack_top));
}

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel)
{
    page_table_t **direntry = get_page_directory_entry(virtual);

    /* page_t *de = (page_t *)direntry; */

    puts("direntry: ");
    puth((uint32_t)direntry);
    putc('\n');

    puts("*direntry: ");
    puth((uint32_t)*direntry);
    putc('\n');

    /* puts("direntry->writeable: "); */
    /* puth(de->writeable); */
    /* putc('\n'); */

    /* puts("direntry->user: "); */
    /* puth(de->user); */
    /* putc('\n'); */

    /* puts("direntry->frame: "); */
    /* puth(de->frame); */
    /* putc('\n'); */

    /* puts("direntry->present: "); */
    /* puth(de->present); */
    /* putc('\n'); */

    puts("virtual: ");
    puth(virtual);
    putc('\n');

    puts("physical: ");
    puth(physical);
    putc('\n');

    puts("DIRINDEX(virtual): ");
    puth(DIRINDEX(virtual));
    putc('\n');

    if (!((uint32_t)(*direntry) & PG_PRESENT)) {
        *direntry =
            (page_table_t *)((alloc_frame() << 12)
                             | PG_USER
                             | PG_PRESENT
                             | PG_WRITABLE);

        if (direntry == NULL) {
            return -ENOMEM;
        }
    }

    page_t *page = get_page(virtual);

    puts("page: ");
    puth((uint32_t)page);
    putc('\n');
 
    if (page->present) {
        return -EINVAL;
    }
    
    page->present = 1;
    page->writeable = !readonly;
    page->user = !kernel;
    page->frame = physical >> 12;

    puts("*page: ");
    puth(*(uint32_t *)page);
    putc('\n');

    puts("page->frame: ");
    puth(page->frame);
    putc('\n');

    puts("page->present: ");
    puth(page->present);
    putc('\n');

    puts("page->writeable: ");
    puth(page->writeable);
    putc('\n');

    puts("page->user: ");
    puth(page->user);
    putc('\n');
    
    read_next_page(virtual);
    
    return 1;
}

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel)
{
    uint32_t physical = alloc_frame();

    puts("allocated physical frame: ");
    puth(physical);
    putc('\n');

    if (!physical) {
        return -ENOMEM;
    }
    
    map_page(virtual, physical, readonly, kernel);
    read_next_page(virtual);

    return 1;
}

void free_page(uint32_t virtual)
{
    memcpy((void *)virtual, &free_stack_top, sizeof(free_stack_top));

    page_t *page = get_page(virtual);
    free_stack_top = (page->frame) << 12;
    
    unmap_page(virtual);
}

/* static page_t *last_free_page(page_directory_t *directory, */
/*                               int kernel, int readonly); */
/* static void map_last_free_page(page_directory_t *directory, uint32_t physical, */
/*                                int kernel, int readonly); */
/* static void map_page(page_t *page, uint32_t physical, int kernel, int readonly); */

/* static void map_page(page_t *page, uint32_t physical, int kernel, int readonly) */
/* { */
/*     page->present = 1; */
/*     page->user = !kernel; */
/*     page->writeable = !readonly; */
/*     page->frame = physical >> 12; */
/* } */

/* static void map_last_free_page(page_directory_t *directory, uint32_t physical, */
/*                                int kernel, int readonly) */
/* { */
/*     page_t *page = last_free_page(directory, kernel, readonly); */
/*     map_page(page, physical, kernel, readonly); */
/* } */

/* static page_t *last_free_page(page_directory_t *directory, */
/*                               int kernel, int readonly) */
/* { */
/*     for (int t = 1022; t >= 0; --t) { */
/*         if (!directory->tables[t]) { */
/*             page_table_t *table = (page_table_t *)alloc_frame(); */

/*             // Point the page directory entry to the table */
/*             directory->tables[t] = table; */

/*             // Mark all pages unmapped */
/*             memset(table, 0, sizeof(page_table_t)); */

/*             // Map one of the pages we just got to the page table */
/*             // Note this will call last_free_page() - but only once! */
/*             map_last_free_page(directory, (uint32_t)table, */
/*                                kernel, readonly); */
/*         } */

/*         for (int p = 1023; p >= 0; --p) { */
/*             page_t *page = &directory->tables[t]->pages[p]; */
/*             if (page->frame == 0) { */
/*                 return page; */
/*             } */
/*         } */
/*     } */

/*     return NULL; */
/* } */

/**
 * note: this takes a physical pointer to directory!
 * only to be used when paging is DISABLED
 */

/* uint32_t alloc_frame() */
/* { */
/*     uint32_t frame = free_stack_pop(); */
/*     return frame; */
/* } */

/* void free_frame(uint32_t page) */
/* { */
/*     free_stack_push(page); */
/* } */

/* static void print_page(page_t page) */
/* { */
/*     puts("p "); */
/*     puth(page.present); */
/*     puts(" w "); */
/*     puth(page.writeable); */
/*     puts(" u "); */
/*     puth(page.user); */
/*     puts(" fr "); */
/*     puth(page.frame); */
/*     putc('\n'); */
/* } */

/* static void print_page_table(page_table_t *table) */
/* { */
/*     puts("page table at: "); */
/*     puth((uint32_t)table); */
/*     putc('\n'); */

/*     for (int i = 0; i < 1024; ++i) { */
/*         if ((i % 256) == 0) { */
/*             puts("page "); */
/*             putui(i); */
/*             puts(": "); */
/*             print_page(table->pages[i]); */
/*         } */
/*     } */
/* } */

/* static void print_page_directory(page_directory_t *directory) */
/* { */
/*     puts("page directory at: "); */
/*     puth((uint32_t)directory); */
/*     putc('\n'); */

/*     for (int i = 0; i < 1024; ++i) { */
/*         puts("table "); */
/*         putui(i); */
/*         puts(": "); */
/*         if (directory->tables[i]) */
/*             print_page_table(directory->tables[i]); */
/*         else */
/*             puts("NULL\n"); */
/*     } */
/* } */

/* void print_free_frame_stack() */
/* { */
/*     uint32_t address = free_stack_top; */
/*     int count = 5; */
/*     puts("FREE PAGE LIST:\n"); */
/*     puth(address); */

/*     while (address != 0 && count > 0) { */
/*         puth((uint32_t)address); */
/*         putc('\n'); */
/*         uint32_t tmp; */
/*         memcpy(&tmp, (void *)address, sizeof(uint32_t)); */
/*         address = tmp; */
/*         --count; */
/*     } */
/* } */

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
