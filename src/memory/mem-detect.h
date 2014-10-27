#ifndef __E820_H_
#define __E820_H_

#include <stdint.h>
#include "mboot.h"

#define E820_MAX_ENTRIES 128

typedef struct bios_mem_map {
    memory_map_t entries[E820_MAX_ENTRIES];
    uint32_t size;
} bios_mem_map_t;

/* extern bios_mem_map_t bios_mem_map; */

/* int init_bios_mem_map(multiboot_info_t *map); */
/* void print_bios_mem_map(); */

int
__attribute__((section (".boot")))
mem_range_is_free(multiboot_info_t *mboot_info,
                  uint32_t start, uint32_t end);

#endif
