#include "address-space.h"

#include "kheap.h"

address_space_t *alloc_address_space()
{
    address_space_t *as = kzalloc(MEM_GEN, sizeof(*as));
    as->pgdir = kzalloc(MEM_GEN, PAGE_SIZE);

    return as;
}
