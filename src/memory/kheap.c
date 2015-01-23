#include "kheap.h"

#include "errno.h"
/* #include "internal.h" */
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
#include "pmm.h"
#include "printf.h"
#include "vmm.h"


/* #define CHUNK_PTR(usr) ((void *)(usr) - sizeof(chunk_t)) */
/* #define USER_PTR(chnk) ((void *)(chnk) + sizeof(chunk_t)) */

/* extern ldsymbol ld_heap_start; */
/* static uint32_t kheap_top; */
/* static chunk_t *first_free_chunk; */
/* static dma_chunk_t *dma_chunks; */
/* static dma_chunk_t *free_dma_chunks; */

/* static uint32_t next_page(uint32_t address) */
/* { */
/*     if (address & 0x00000FFF) { */
/*         address &= 0xFFFFF000; */
/*         address += PAGE_SIZE; */
/*     } */

/*     return address; */
/* } */

/* static chunk_t *split_chunk(chunk_t *chunk, uint32_t size) */
/* { */
/*     if (size + sizeof(*chunk) >= chunk->size) { */
/*         PANIC("Tried to split a chunk into larger parts than the whole!"); */
/*     } */

/*     chunk_t *new = USER_PTR(chunk) + size; */
/*     new->size = chunk->size - sizeof(*chunk) - size; */
/*     chunk->size = size; */

/*     return new; */
/* } */

/* static void combine_chunks(chunk_t *first, chunk_t *second) */
/* { */
/*     first->size += second->size + sizeof(*second); */
/* } */

/* static uint8_t can_combine_chunks(chunk_t *first, chunk_t *second) */
/* { */
/*     if (USER_PTR(first) + first->size == (void *)second) { */
/*         return 1; */
/*     } */

/*     return 0; */
/* } */

/* static void defrag_after(chunk_t *chunk) { */
/*     while (can_combine_chunks(chunk, chunk->next)) { */
/*         combine_chunks(chunk, chunk->next); */

/*         chunk->next->prev = chunk; */
/*         chunk->next = chunk->next->next; */
/*     } */
/* } */

/* static void insert_after(chunk_t *chunk, chunk_t *new) { */
/*     if (chunk->next) { */
/*         chunk->next->prev = new; */
/*     } */

/*     new->next = chunk->next; */
/*     new->prev = chunk; */
/*     chunk->next = new; */

/*     defrag_after(chunk); */
/* } */

/* static void insert_before(chunk_t *chunk, chunk_t *new) { */
/*     if (chunk == first_free_chunk) { */
/*         first_free_chunk = new; */
/*     } */

/*     if (chunk->prev) { */
/*         chunk->prev->next = new; */
/*     } */
    
/*     new->prev = chunk->prev; */
/*     new->next = chunk; */
/*     chunk->prev = new; */

/*     defrag_after(new); */
/* } */

/* static chunk_t *remove_chunk(chunk_t *chunk) { */
/*     if (chunk->next) { */
/*         chunk->next->prev = chunk->prev; */
/*     } */
/*     if (chunk->prev) { */
/*         chunk->prev->next = chunk->next; */
/*     } */
/*     if (chunk == first_free_chunk) { */
/*         first_free_chunk = chunk->next; */
/*     } */

/*     return chunk; */
/* } */

/* static void insert_chunk(chunk_t *chunk) */
/* { */
/*     /\* print_chunk(chunk, "inserting"); *\/ */
/*     chunk_t *free = first_free_chunk; */

/*     if (!free) { */
/*         /\* print_chunk(chunk, "inserting first"); *\/ */
/*         chunk->next = NULL; */
/*         chunk->prev = NULL; */
/*         first_free_chunk = chunk; */
/*     } */
/*     else { */
/*         while (free) { */
/*             if (free->size >= chunk->size) { */
/*                 insert_before(free, chunk); */
/*                 break; */
/*             } */
/*             if (free->next == NULL) { */
/*                 insert_after(free, chunk); */
/*                 break; */
/*             } */

/*             free = free->next; */
/*         } */
/*     } */
/* } */

/* static void *kmalloc_large(uint32_t size) */
/* { */
/*     uint32_t alloc_size = size + sizeof(chunk_t); */
/*     uint32_t num_pages; */
/*     uint32_t aligned_size = next_page(alloc_size); */
    
/*     num_pages = aligned_size / PAGE_SIZE; */

/*     int err = alloc_pages(kheap_top, 0, 1, num_pages); */
/*     if (err < 0) { */
/*         errno = ENOMEM; */
/*         return NULL; */
/*     } */

/*     chunk_t *chunk = (chunk_t *)kheap_top; */
/*     chunk->size = aligned_size - sizeof(chunk_t); */
/*     chunk->next = NULL; */
/*     chunk->prev = NULL; */

/*     if (size < chunk->size - sizeof(chunk_t)) { */
/*         chunk_t *new = split_chunk(chunk, size); */
/*         insert_chunk(new); */
/*     } */

/*     kheap_top += aligned_size; */
    
/*     return USER_PTR(chunk); */
/* } */

/* static void insert_dma(dma_chunk_t **list, dma_chunk_t *chunk) */
/* { */
/*     if (!*list) { */
/*         *list = chunk; */
/*         chunk->next = NULL; */
/*         return; */
/*     } */

/*     dma_chunk_t *curr = *list; */
/*     while (curr->next) { */
/*         curr = curr->next; */
/*     } */

/*     curr->next = chunk; */
/*     chunk->next = NULL; */
/* } */

/* static dma_chunk_t *remove_dma(dma_chunk_t **list, uint32_t virtual) */
/* { */
/*     dma_chunk_t *curr = *list; */
/*     dma_chunk_t *next = curr->next; */

/*     if (curr->virtual == virtual) { */
/*         *list = next; */
/*         return curr; */
/*     } */

/*     while (next) { */
/*         if (next->virtual == virtual) { */
/*             curr->next = next->next; */
/*             return next; */
/*         } */
/*     } */

/*     return NULL; */
/* } */

/* static bool dma_region_ok(uint32_t physical, uint32_t size) */
/* { */
/*     if ((physical & 0xFFFF) + size > 0xFFFF) { */
/*         return false; */
/*     } */
/*     else { */
/*         return true; */
/*     } */
/* } */

/* static dma_chunk_t *find_dma(dma_chunk_t *list, uint32_t size) */
/* { */
/*     while (list) { */
/*         uint32_t physical = get_physical(list->virtual); */
/*         if (list->size > size && dma_region_ok(physical, size)) { */
/*             return list; */
/*         } */
/*     } */

/*     return NULL; */
/* } */

/* static dma_chunk_t *split_dma(dma_chunk_t *chunk, uint32_t size) */
/* { */
/*     if (size >= chunk->size) { */
/*         PANIC("Tried to split DMA chunk into parts larger than the whole!"); */
/*     } */

/*     dma_chunk_t *new = kzalloc(MEM_GEN, sizeof(*new)); */
/*     chunk->size -= size; */
/*     new->size = size; */
/*     new->virtual = chunk->virtual + size; */

/*     return new; */
/* } */

/* static void *kmalloc_dma(uint32_t size) */
/* { */
/*     size = align(size, PAGE_SIZE); */

/*     dma_chunk_t *chunk = find_dma(free_dma_chunks, size); */

/*     if (chunk) { */
/*         remove_dma(&free_dma_chunks, chunk->virtual); */
/*         dma_chunk_t *new = split_dma(chunk, size); */
/*         insert_dma(&free_dma_chunks, new); */
/*         insert_dma(&dma_chunks, chunk); */
/*         return (void *)(chunk->virtual); */
/*     } */

/*     int err = dma_alloc_pages(kheap_top, false, true, size / PAGE_SIZE); */
/*     if (err < 0) { */
/*         errno = ENOMEM; */
/*         return NULL; */
/*     } */

/*     uint32_t addr = kheap_top; */
/*     kheap_top += size; */

/*     chunk = kzalloc(MEM_GEN, sizeof(*chunk)); */
/*     chunk->size = size; */
/*     chunk->virtual = addr; */
/*     insert_dma(&dma_chunks, chunk); */

/*     return (void *)(chunk->virtual); */
/* } */

/* static void kfree_dma(void *virtual) */
/* { */
/*     dma_chunk_t *chunk = remove_dma(&dma_chunks, (uint32_t)virtual); */
/*     if (!chunk) { */
/*         PANIC("Unable to find DMA chunk to free!"); */
/*     } */

/*     insert_dma(&free_dma_chunks, chunk); */
/* } */

/* void *kmalloc(kmem_type_t type, uint32_t size) */
/* { */
/*     if (type == MEM_DMA) { */
/*         return kmalloc_dma(size); */
/*     } */

/*     printf("kmalloc "); */
/*     size = align(size, 4); */

/*     printf("aligned "); */
/*     if (size > PAGE_SIZE - sizeof(chunk_t)) { */
/*         return kmalloc_large(size); */
/*     } */

/*     printf("not large "); */
/*     chunk_t *chunk = first_free_chunk; */

/*     while (chunk) { */
/*         printf("checking %x ", chunk); */
/*         if (chunk->size == size) { */
/*             printf("right size\n"); */
/*             return USER_PTR(remove_chunk(chunk)); */
/*         } */
/*         if (chunk->size > size + sizeof(*chunk)) { */
/*             printf("too big\n"); */
/*             remove_chunk(chunk); */
/*             chunk_t *new = split_chunk(chunk, size); */
/*             insert_chunk(new); */
/*             return USER_PTR(chunk); */
/*         } */

/*         chunk = chunk->next; */
/*     } */

/*     printf("no mem "); */
/*     int err = alloc_page(kheap_top, 0, 1); */
/*     if (err < 0) { */
/*         errno = ENOMEM; */
/*         return NULL; */
/*     } */

/*     printf("new page "); */
/*     chunk_t *ret = (chunk_t *)kheap_top; */
/*     ret->size = PAGE_SIZE - sizeof(*ret); */

/*     kheap_top += PAGE_SIZE; */

/*     chunk_t *new = split_chunk(ret, size); */
/*     insert_chunk(new); */
/*     printf("split\n"); */
/*     return USER_PTR(ret); */
/* } */

/* void *kcalloc(kmem_type_t type, uint32_t num, uint32_t size) */
/* { */
/*     void *ret = kmalloc(type, size * num); */
/*     printf("kmalloc returned %x to kcalloc "); */
/*     memset(ret, 0, size * num); */
/*     printf("kcalloc zeroed mem\n"); */
/*     return ret; */
/* } */

/* void *kzalloc(kmem_type_t type, uint32_t size) */
/* { */
/*     printf("kzalloc called "); */
/*     void *ret = kcalloc(type, 1, size); */
/*     return ret; */
/* } */

/* void kfree(void *address) */
/* { */
/*     if (!address) { */
/*         return; */
/*     } */

/*     if (check_dma_address((uint32_t)address)) { */
/*         kfree_dma(address); */
/*     } */

/*     chunk_t *chunk = CHUNK_PTR(address); */
/*     chunk->next = NULL; */
/*     chunk->prev = NULL; */
/*     insert_chunk(chunk); */
/* } */

/* void init_kheap(void) */
/* { */
/*     puts("Initializing kernel heap...\n"); */
/*     kheap_top = next_page((uint32_t)ld_heap_start); */
    
/*     int err = alloc_page(kheap_top, 0, 1); */
/*     if (err < 0) { */
/*         PANIC("Unable to allocate pages for the kernel heap!"); */
/*     } */

/*     chunk_t *chunk = (chunk_t *)kheap_top; */
/*     chunk->size = PAGE_SIZE - sizeof(*chunk); */

/*     insert_chunk(chunk); */

/*     kheap_top += PAGE_SIZE; */
/*     puts("Initialized kernel heap!\n"); */
/* } */
