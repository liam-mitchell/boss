#ifndef __KHEAP_H_
#define __KHEAP_H_

#include <stdint.h>

void *kmalloc(uint32_t size);
void kfree(void *address);

void init_kheap();

#endif
