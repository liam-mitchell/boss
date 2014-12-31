#ifndef __MM_H_
#define __MM_H_

#include <stdint.h>

#define PAGE_SIZE 0x1000

#define DIRINDEX(virtual) ((virtual) >> 22)
#define TBLINDEX(virtual) (((virtual) >> 12) & 0x3FF)

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

uint32_t **get_page_directory_entry(uint32_t virtual);
uint32_t *get_page_table(uint32_t virtual);
uint32_t *get_page(uint32_t virtual);

uint32_t map_physical(uint32_t physical);

void init_paging();

#endif // __MM_H_
