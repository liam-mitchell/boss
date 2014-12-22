#ifndef __MM_H_
#define __MM_H_

#include <stdint.h>

#define PAGE_SIZE 0x1000

#define DIRINDEX(virtual) ((virtual) >> 22)
#define TBLINDEX(virtual) (((virtual) >> 12) & 0x3FF)

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

typedef struct address_space {
    page_directory_t *pgdir;

    uint32_t stack_top;
    uint32_t esp;
    uint32_t prog_break;
} address_space_t;

page_directory_t kdirectory;

uint32_t alloc_frame();
void free_frame(uint32_t address);

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel);
int alloc_pages(uint32_t virtual, uint8_t readonly,
                uint8_t kernel, uint32_t num);
void free_page(uint32_t virtual);

void clone_address_space(address_space_t *dest, const address_space_t *src);
void switch_address_space(address_space_t *old, const address_space_t *new);

uint32_t map_physical(uint32_t physical);

uint32_t top();
uint32_t top_addr();

void init_paging();

#endif // __MM_H_
