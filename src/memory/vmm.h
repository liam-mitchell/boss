#ifndef __VMM_H_
#define __VMM_H_

#include <stdint.h>

#include "bool.h"
#include "compiler.h"

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel);
int alloc_pages(uint32_t virtual, uint8_t readonly,
                uint8_t kernel, uint32_t num);
int dma_alloc_pages(uint32_t virtual, bool readonly, bool kernel, uint32_t n);
void dma_free_pages(uint32_t virtual, uint32_t n);

void free_page(uint32_t virtual);
void set_page_attributes(uint32_t *page, bool present,
                         bool writeable, bool user);

uint32_t **get_page_directory_entry(uint32_t virtual);
uint32_t *get_page_table(uint32_t virtual);
uint32_t *get_page(uint32_t virtual);

uint32_t map_physical(uint32_t physical);
uint32_t get_physical(uint32_t virtual);
void unmap_page(uint32_t virtual);

bool check_user_ptr(void __user *ptr);

void flush_tlb(uint32_t virtual);

void init_paging();

#endif /* __VMM_H_ */
