#include "address-space.h"

#include "kheap.h"

address_space_t *alloc_address_space()
{
    address_space_t *as = kzalloc(sizeof(*as));
    as->pgdir = kzalloc(PAGE_SIZE);

    return as;
}
