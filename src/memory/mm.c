#include "mm.h"
#include "memory.h"
#include "mem-detect.h"
#include "terminal.h"

#include <stdint.h>

#define PAGE_SIZE 0x1000
#define for_each_frame(address)                                                 \
    for (address = 0;                                                           \
         address < 0xFFFFF000;                                                  \
         address += PAGE_SIZE)

#define DIRINDEX(virtual) virtual >> 22
#define TBLINDEX(virtual) (virtual >> 12) & 0x3FF

extern uint32_t _end;
extern uint32_t kernel_physical_end;

static uint32_t kernel_end = (uint32_t)&_end;

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

static page_directory_t kernel_directory;

/* static page_t *last_free_page(page_directory_t *directory, */
/*                               int kernel, int readonly); */
/* static void map_last_free_page(page_directory_t *directory, uint32_t physical, */
/*                                int kernel, int readonly); */
/* static void map_page(page_t *page, uint32_t physical, int kernel, int readonly); */

/* static void identity_map(page_directory_t *directory, */
/*                          uint32_t start, uint32_t end) */
/* { */
/*     uint32_t physical = start & 0xFFFFF000; */

/*     while (physical <= end) { */
/*         page_table_t *table = directory->tables[DIRINDEX(physical)]; */
/*         if (!table) { */
/*             table = (page_table_t *)alloc_frame(); */
/*             directory->tables[DIRINDEX(physical)] = table; */
/*             memset(table, 0, sizeof(page_table_t)); */
/*             map_last_free_page(directory, (uint32_t)table, */
/*                                1, 0); */
/*         } */

/*         page_t *page = &table->pages[TBLINDEX(physical)]; */
/*         map_page(page, physical, 1, 0); */

/*         physical += PAGE_SIZE; */
/*     } */
/* } */

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

/* static */
/* __attribute__((section (".boot"))) */
/* void free_stack_push(uint32_t address) */
/* { */
/*     memcpy((void *)address, &free_stack_top, sizeof(uint32_t)); */
/*     free_stack_top = address; */
/* } */

/* static uint32_t free_stack_pop() */
/* { */
/*     uint32_t address = free_stack_top; */
/*     memcpy(&free_stack_top, (void *)free_stack_top, sizeof(uint32_t)); */
/*     return address; */
/* } */

/**
 * note: this takes a physical pointer to directory!
 * only to be used when paging is DISABLED
 */
/* static void recursive_map_dir(page_directory_t *directory) */
/* { */
/*     puts("Mapping last entry...\n"); */
/*     putc('\n'); */
/*     putc('\n'); */
/*     putc('\n'); */
/*     directory->tables[1023] = (page_table_t *)directory; */
/*     puts("Mapped last entry!\n"); */
/* } */

/* /\** */
/*  * note: this takes a physical pointer to directory! */
/*  * only to be used when paging is DISABLED */
/*  * (not that that wasn't obvious...) */
/*  *\/ */
/* static void enable_paging(page_directory_t *directory) */
/* { */
/*     puts("Enabling paging...\n"); */
/*     asm volatile ("mov %0, %%cr3" : : "r"(directory)); */
/*     uint32_t cr0; */
/*     asm volatile ("mov %%cr0, %0" : "=r"(cr0)); */
/*     cr0 |= (1 << 31); */
/*     asm volatile ("mov %0, %%cr0" : : "r"(cr0)); */

/*     asm(".intel_syntax noprefix"); */

/*     asm volatile ( */
/*                   "lea ecx, [higherhalf]\n\t" */
/*                   "jmp ecx\n" */
/*                   "higherhalf:\n\t" */
/*                   : */
/*                   : */
/*                   : "ecx" */
/*                   ); */

/*     asm(".att_syntax noprefix"); */

/*     while(1) {} */

/*     puts("Set CR0!\n"); */
/*     putc('\n'); */
/*     putc('\n'); */
/*     putc('\n'); */
/* } */

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
                  uint32_t start, uint32_t end);


#define for_each_entry(entry, end, mboot)                                       \
    for (entry = (memory_map_t *)(mboot->mmap_addr),                            \
                       end = entry + mboot_info->mmap_length;                   \
    entry < end;                                                                \
    entry = (memory_map_t *)((unsigned int)entry                                \
                             + entry->size                                      \
                             + sizeof(unsigned long)))                          

static inline int
__attribute__((section(".boot")))
mem_range_is_free(multiboot_info_t *mboot_info,
                  uint32_t start, uint32_t end)
{
    if (mboot_info->flags & (1 << 6)) {
        memory_map_t *entry;
        memory_map_t *last_entry;
        for_each_entry(entry, last_entry, mboot_info) {
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

uint32_t
__attribute__((section (".text")))
init_free_frame_stack(multiboot_info_t *mboot_info)
{
    free_stack_top = 0;

    uint32_t address;
    uint32_t count = 0;

    if (!(mboot_info->flags & (1 << 6))) {
        return 0;
    }

    for_each_frame(address) {
        if (address < (uint32_t)&kernel_physical_end) {
            continue;
        }
        
        if (mem_range_is_free(mboot_info, address, address + PAGE_SIZE)) {
            *(uint32_t *)address = free_stack_top;
            free_stack_top = address;
            ++count;
        }
    }

    return count;
}
/* void init_paging() */
/* { */
/*     init_free_frame_stack(); */
/*     puts("Mapping kernel directory to itself...\n"); */
/*     recursive_map_dir(&kernel_directory); */

/*     /\* print_page_directory(&kernel_directory); *\/ */
/*     enable_paging(&kernel_directory); */
/* } */

/* uint32_t top() */
/* { */
/*     return kernel_end; */
/* } */
