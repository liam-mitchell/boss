#include "address-space.h"

#include "kheap.h"
#include "ldsymbol.h"
#include "mm.h"

extern ldsymbol ld_virtual_start;
extern ldsymbol ld_virtual_offset;

address_space_t *alloc_address_space()
{
    address_space_t *as = kzalloc(sizeof(*as));
    as->pgdir = kzalloc(PAGE_SIZE);

    for (uint32_t virtual = (uint32_t)ld_virtual_offset;
         virtual < 0xFFFFF000;
         virtual += PAGE_SIZE)
    {
        as->pgdir[DIRINDEX(virtual)] = get_page_table(virtual);
    }

    as->pgdir[0x3FF] =
        (uint32_t *)((uint32_t)as->pgdir - (uint32_t)ld_virtual_offset);

    as->brk = 0;

    return as;
}
