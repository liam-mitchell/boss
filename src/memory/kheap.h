#ifndef __KHEAP_H_
#define __KHEAP_H_

#include <stdint.h>

void *kmalloc(uint32_t size);
void *kcalloc(uint32_t num, uint32_t size);
void *kzalloc(uint32_t size);
void kfree(void *address);

void init_kheap(void);
void print_chunk_list(void);

#endif
