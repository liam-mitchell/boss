#ifndef __KHEAP_H_
#define __KHEAP_H_

#include <stdint.h>

typedef enum {
    MEM_GEN,
    MEM_DMA
} kmem_type_t;

void *kmalloc(kmem_type_t type, uint32_t size);
void *kcalloc(kmem_type_t type, uint32_t num, uint32_t size);
void *kzalloc(kmem_type_t type, uint32_t size);
void kfree(void *address);

void init_kheap(void);
void print_chunk_list(void);

#endif
