// memory_map.h
// int 15h functions for querying bios

#ifndef __MEMORY_MAP_H_
#define __MEMORY_MAP_H_

#include <stdint.h>
#include "bool.h"

typedef struct memory_region_t
{
	uint32_t base_low;
	uint32_t base_high;
	uint32_t length_low;
	uint32_t lenght_high;
	uint32_t type;
	uint32_t one;
} memory_region_t;

memory_region_t memory_map[1024];

extern uint32_t init_e820h(memory_region_t *start);
extern bool next_e820h();

#endif // __MEMORY_MAP_H_
