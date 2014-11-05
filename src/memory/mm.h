#ifndef __MM_H_
#define __MM_H_

#include <stdint.h>

#define PAGE_SIZE 0x1000

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel);
void free_page(uint32_t virtual);

uint32_t top();
uint32_t top_addr();

#endif // __MM_H_
