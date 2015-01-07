#ifndef __ADDRESS_SPACE_H_
#define __ADDRESS_SPACE_H_

#include "pmm.h"

typedef struct address_space {
    uint32_t **pgdir;

    uint32_t brk;
} address_space_t;

address_space_t *alloc_address_space();

#endif // __ADDRESS_SPACE_H_
