#ifndef __MM_H_
#define __MM_H_

#include "bool.h"
#include "compiler.h"

#include <stdint.h>

#define PAGE_SIZE 0x1000

#define DIRINDEX(virtual) ((virtual) >> 22)
#define TBLINDEX(virtual) (((virtual) >> 12) & 0x3FF)

#define PG_PRESENT (1 << 0)
#define PG_WRITEABLE (1 << 1)
#define PG_USER (1 << 2)
#define PG_ACCESSED (1 << 3)
#define PG_DIRTY (1 << 4)

#define PG_FRAME(p) ((p) & ~0xFFF)
#define PG_IS_PRESENT(p) ((p) & PG_PRESENT)
#define PG_IS_WRITEABLE(p) ((p) & PG_WRITEABLE)
#define PG_IS_USERMODE(p) ((p) & PG_USER)
#define PG_IS_ACCESSED(p) ((p) & PG_ACCESSED)
#define PG_IS_DIRTY(p) ((p) & PG_DIRTY)

#define for_each_frame(address)                                                 \
    for (address = 0;                                                           \
         address < 0xFFFFF000;                                                  \
         address += PAGE_SIZE)

/* typedef struct page { */
/*     uint32_t present : 1; */
/*     uint32_t writeable : 1; */
/*     uint32_t user : 1; */
/*     uint32_t accessed : 1; */
/*     uint32_t dirty : 1; */
/*     uint32_t unused : 7; */
/*     uint32_t frame : 20; */
/* } page_t; */

/* typedef struct page_table { */
/*     page_t pages[1024]; */
/* } page_table_t; */

/* typedef struct page_directory { */
/*     page_table_t *tables[1024]; */
/* } page_directory_t; */

uint32_t kdirectory;

uint32_t alloc_frame();
void free_frame(uint32_t address);

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel);
int alloc_pages(uint32_t virtual, uint8_t readonly,
                uint8_t kernel, uint32_t num);
void free_page(uint32_t virtual);
void set_page_attributes(uint32_t *page, bool present,
                         bool writeable, bool user);

uint32_t **get_page_directory_entry(uint32_t virtual);
uint32_t *get_page_table(uint32_t virtual);
uint32_t *get_page(uint32_t virtual);

uint32_t map_physical(uint32_t physical);
void unmap_page(uint32_t virtual);

bool check_user_ptr(void __user *ptr);

void flush_tlb(uint32_t virtual);

void init_paging();

#endif // __MM_H_
