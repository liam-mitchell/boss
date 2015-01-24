#include "kheap.h"

#include "internal.h"
#include "errno.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
#include "pmm.h"
#include "printf.h"
#include "vmm.h"

extern ldsymbol ld_heap_start;

static struct chunk *find_chunk(struct chunk *list, unsigned long size)
{
    while (list) {
        if (list->size >= size) {
            return list;
        }

        list = list->next;
    }

    return NULL;
}

static struct chunk *remove_chunk(struct chunk **list, struct chunk *chunk)
{
    if (!list || !chunk) {
        return NULL;
    }

    if (*list == chunk) {
        *list = chunk->next;
    }
    
    if (chunk->next) {
        chunk->next->prev = chunk->prev;
    }

    if (chunk->prev) {
        chunk->prev->next = chunk->next;
    }

    return chunk;
}

static struct chunk *split_chunk(struct chunk *chunk, unsigned long size)
{
    if (size + sizeof(unsigned long) > chunk->size) {
        return NULL;
    }

    unsigned long new_size = chunk->size - sizeof(unsigned long) - size;
    struct chunk *new = (struct chunk *)(USER_PTR(chunk) + size);

    chunk->size = size;

    new->size = new_size;
    new->next = NULL;
    new->prev = NULL;

    return new;
}

static void defrag_before(struct chunk *chunk)
{
    while (chunk->prev
           && (USER_PTR(chunk->prev) + chunk->prev->size == chunk))
    {
        chunk->prev->size += chunk->size + sizeof(unsigned long);
        chunk->prev->next = chunk->next;

        if (chunk->next) {
            chunk->next->prev = chunk->prev;
        }

        chunk = chunk->prev;
    }
}

static void defrag_after(struct chunk *chunk)
{
    while (chunk->next
           && (USER_PTR(chunk) + chunk->size == chunk->next))
    {
        chunk->size += chunk->next->size + sizeof(unsigned long);
        chunk->next = chunk->next->next;

        if (chunk->next) {
            chunk->next->prev = chunk;
        }
    }
}

static void add_before(struct chunk *chunk, struct chunk *new)
{
    if (chunk->prev) {
        chunk->prev->next = new;
    }

    new->prev = chunk->prev;
    new->next = chunk;

    chunk->prev = new;
}

static void add_after(struct chunk *chunk, struct chunk *new)
{
    if (chunk->next) {
        chunk->next->prev = new;
    }

    new->next = chunk->next;
    new->prev = chunk;

    chunk->next = new;
}

static void add_chunk(struct chunk **list, struct chunk *chunk)
{
    if (!list) {
        return;
    }

    struct chunk *curr = *list;

    chunk->next = NULL;
    chunk->prev = NULL;

    if (!curr) {
        *list = chunk;
        defrag_after(chunk);
        return;
    }

    if (curr > chunk) {
        *list = chunk;

        add_before(curr, chunk);
        defrag_after(chunk);
        return;
    }

    while (curr) {
        if (curr > chunk) {
            add_before(curr, chunk);
            break;
        }
        else if (!curr->next) {
            add_after(curr, chunk);
            break;
        }

        curr = curr->next;
    }

    defrag_after(chunk);
    defrag_before(chunk);
}

static void *alloc_chunk(unsigned long size)
{
    int err = alloc_page(kheap_top, 0, 1);
    if (err < 0) {
        errno = -ENOMEM;
        return NULL;
    }

    struct chunk *new = (struct chunk *)kheap_top;
    kheap_top += PAGE_SIZE;

    new->size = PAGE_SIZE - sizeof(unsigned long);
    new->next = NULL;
    new->prev = NULL;

    if (new->size > size) {
        struct chunk *split = split_chunk(new, size);
        add_chunk(&free, split);
    }

    return USER_PTR(new);
}

static void *kmalloc_general(unsigned long size)
{
    struct chunk *curr = find_chunk(free, size);
    if (!curr) {
        return alloc_chunk(size);
    }

    remove_chunk(&free, curr);

    if (curr->size > size + sizeof(*curr)) {
        struct chunk *new = split_chunk(curr, size);
        add_chunk(&free, new);
    }

    return USER_PTR(curr);
}

static void *kmalloc_large(unsigned long size)
{
    unsigned long alloc_size = size + sizeof(struct chunk);
    unsigned long aligned_size = align(alloc_size, PAGE_SIZE);
    unsigned long num_pages = aligned_size / PAGE_SIZE;

    int err = alloc_pages(kheap_top, 0, 1, num_pages);
    if (err < 0) {
        errno = ENOMEM;
        return NULL;
    }

    struct chunk *chunk = (struct chunk *)kheap_top;
    chunk->size = aligned_size - sizeof(struct chunk);
    chunk->next = NULL;
    chunk->prev = NULL;

    if (size < chunk->size - sizeof(struct chunk)) {
        struct chunk *new = split_chunk(chunk, size);
        add_chunk(&free, new);
    }

    kheap_top += aligned_size;
    
    return USER_PTR(chunk);
}

void *kmalloc(kmem_type_t type, unsigned long size)
{
    if (size == 0) {
        return NULL;
    }
    
    if (type == MEM_DMA) {
        return kmalloc_dma(size);
    }
    else if (type != MEM_GEN) {
        return NULL;
    }

    size = align(size, sizeof(unsigned long) * 2);
    if (size > PAGE_SIZE - sizeof(struct chunk)) {
        return kmalloc_large(size);
    }

    return kmalloc_general(size);
}

void kfree(void *address)
{
    if (!address) {
        return;
    }

    if (check_dma_address((unsigned long)address)) {
        kfree_dma(address);
        return;
    }

    struct chunk *chunk = CHUNK_PTR(address);

    chunk->next = NULL;
    chunk->prev = NULL;
    add_chunk(&free, chunk);
}

void *kcalloc(kmem_type_t type, uint32_t num, uint32_t size)
{
    void *ret = kmalloc(type, size * num);
    memset(ret, 0, size * num);
    return ret;
}

void *kzalloc(kmem_type_t type, uint32_t size)
{
    void *ret = kcalloc(type, 1, size);
    return ret;
}

void init_kheap(void)
{
    puts("Initializing kernel heap...\n");
    kheap_top = align((unsigned long)ld_heap_start, PAGE_SIZE);
    
    int err = alloc_page(kheap_top, 0, 1);
    if (err < 0) {
        PANIC("Unable to allocate pages for the kernel heap!");
    }

    struct chunk *chunk = (struct chunk *)kheap_top;
    chunk->size = PAGE_SIZE - sizeof(unsigned long);

    free = NULL;
    dma = NULL;
    free_dma = NULL;

    add_chunk(&free, chunk);

    kheap_top += PAGE_SIZE;
    puts("Initialized kernel heap!\n");
}
