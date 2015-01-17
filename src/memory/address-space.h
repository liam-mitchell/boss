#ifndef __ADDRESS_SPACE_H_
#define __ADDRESS_SPACE_H_

#include "pmm.h"

typedef struct address_space {
    uint32_t **pgdir;

    uint32_t brk;
} address_space_t;

address_space_t *alloc_address_space();
void free_address_space(address_space_t *as);
address_space_t *clone_address_space();
void switch_address_space(address_space_t *old, const address_space_t *new);
void save_address_space(address_space_t *as);

#endif // __ADDRESS_SPACE_H_
