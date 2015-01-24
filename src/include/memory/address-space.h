#ifndef __ADDRESS_SPACE_H_
#define __ADDRESS_SPACE_H_

#include "memory/pmm.h"

typedef struct address_space {
    uint32_t **pgdir;

    uint32_t brk;
} address_space_t;

address_space_t *alloc_address_space();
void free_address_space(address_space_t *as);
address_space_t *clone_address_space();
void switch_address_space(address_space_t *old, const address_space_t *new);
void save_address_space(address_space_t *as);

int map_as_data(address_space_t *as, uint32_t len, void *data);
int map_as_stack(address_space_t *as);

#endif // __ADDRESS_SPACE_H_
