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

/* uint32_t kdirectory; */

uint32_t _alloc_frame();
uint32_t alloc_frame();
uint32_t dma_alloc_frames(uint32_t n);
void free_frame(uint32_t address);
void dma_free_frames(uint32_t physical, uint32_t n);
bool check_dma_address(uint32_t physical);

#endif // __MM_H_
