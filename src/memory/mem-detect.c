#include "mem-detect.h"
#include "memory.h"
#include "terminal.h"
#include "mboot.h"

#define MEM_USABLE 1
#define MEM_RESERVED 2
#define MEM_RECLAIMABLE 3
#define MEM_NVS 4
#define MEM_BAD 5

bios_mem_map_t bios_mem_map;

/* static void print_bios_mem_type(uint32_t type) */
/* { */
/*     if (type == MEM_USABLE) */
/*         puts("usable"); */
/*     else if (type == MEM_RESERVED) */
/*         puts("reserved"); */
/*     else if (type == MEM_RECLAIMABLE) */
/*         puts("ACPI reclaimable"); */
/*     else if (type == MEM_NVS) */
/*         puts("ACPI NVS"); */
/*     else if (type == MEM_BAD) */
/*         puts("badmem"); */
/*     else */
/*         puts("badtype"); */
/* } */

/* static void print_bios_mem_entry(memory_map_t *entry) */
/* { */
/*     puth(entry->base_addr_high); */
/*     putuh(entry->base_addr_low); */
/*     puts(" - "); */

/*     unsigned long end_addr_high = */
/*         entry->base_addr_high + entry->length_high; */
/*     unsigned long end_addr_low = */
/*         entry->base_addr_low + entry->length_low; */

/*     if (end_addr_low < entry->base_addr_low) { */
/*         ++end_addr_high; */
/*     } */

/*     puth(end_addr_high); */
/*     putuh(end_addr_low); */

/*     puts(" ("); */
/*     puth(entry->length_high); */
/*     putuh(entry->length_low); */
/*     puts(") ["); */
/*     print_bios_mem_type(entry->type); */
/*     puts("]\n"); */
/* } */

/**
 * NOTE: this only covers up to 4G of memory! GRUB may return more entries
 * past the end, but we don't really care - we can't address them yet
 * anyways!
/* int mem_range_is_free(uint32_t start, uint32_t end) */
/* { */
/*     for (unsigned int i = 0; i < bios_mem_map.size; ++i) { */
/*         if (bios_mem_map.entries[i].type != MEM_USABLE) */
/*             continue; */

/*         uint32_t entry_start = bios_mem_map.entries[i].base_addr_low; */
/*         uint32_t entry_end = entry_start + bios_mem_map.entries[i].length_low; */

/*         if (entry_end < start) */
/*             continue; */

/*         if (entry_start < start) { */
/*             if (entry_end > end) // this usable region covers start -> end */
/*                 return 1; */
/*             else // otherwise, we covered at least start -> entry_end */
/*                 start = entry_end; */
/*         } */
/*         else return 0; // we're past end, and haven't found it to be good */
/*     } */

/*     return 0; */
/* } */

/* void print_bios_mem_map() */
/* { */
/*     for (unsigned int i = 0; i < bios_mem_map.size; ++i) { */
/*         print_bios_mem_entry(&bios_mem_map.entries[i]); */
/*     } */
/* } */

/* void init_bios_mem_map(multiboot_info_t *mboot_info) */
/* { */
/*     if (mboot_info->flags & (1 << 6)) { */
/*         memory_map_t *entry = (memory_map_t *)(mboot_info->mmap_addr); */
/*         memory_map_t *end = entry + mboot_info->mmap_length; */

/*         while (entry < end && bios_mem_map.size < E820_MAX_ENTRIES) */
/*         { */
/*             memcpy(&bios_mem_map.entries[bios_mem_map.size], */
/*                    entry, */
/*                    sizeof(memory_map_t)); */

/*             ++bios_mem_map.size; */

/*             entry = (memory_map_t *)((unsigned int)entry */
/*                                      + entry->size */
/*                                      + sizeof(unsigned long)); */
/*         } */

/*         return bios_mem_map.size; */
/*     } */
/*     else { */
/*         puts("BIOS memory map invalid!\n"); */
/*         return -1; */
/*     } */
/* } */
