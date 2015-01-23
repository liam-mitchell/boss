#include "kheap.h"

#include "internal.h"
#include "errno.h"
#include "macros.h"
#include "pmm.h"
#include "printf.h"
#include "vmm.h"

static void insert_dma(struct dma_chunk **list, struct dma_chunk *chunk)
{
    if (!*list) {
        *list = chunk;
        chunk->next = NULL;
        return;
    }

    struct dma_chunk *curr = *list;
    while (curr->next) {
        curr = curr->next;
    }

    curr->next = chunk;
    chunk->next = NULL;
}

static struct dma_chunk *remove_dma(struct dma_chunk **list, unsigned long virtual)
{
    struct dma_chunk *curr = *list;
    struct dma_chunk *next = curr->next;

    if (curr->virtual == virtual) {
        *list = next;
        return curr;
    }

    while (next) {
        if (next->virtual == virtual) {
            curr->next = next->next;
            return next;
        }
    }

    return NULL;
}

static bool dma_region_ok(unsigned long physical, unsigned long size)
{
    if ((physical & 0xFFFF) + size > 0xFFFF) {
        return false;
    }
    else {
        return true;
    }
}

static struct dma_chunk *find_dma(struct dma_chunk *list, unsigned long size)
{
    while (list) {
        unsigned long physical = get_physical(list->virtual);
        if (list->size > size && dma_region_ok(physical, size)) {
            return list;
        }
    }

    return NULL;
}

static struct dma_chunk *split_dma(struct dma_chunk *chunk, unsigned long size)
{
    if (size >= chunk->size) {
        PANIC("Tried to split DMA chunk into parts larger than the whole!");
    }

    struct dma_chunk *new = kzalloc(MEM_GEN, sizeof(*new));
    chunk->size -= size;
    new->size = size;
    new->virtual = chunk->virtual + size;

    return new;
}

void *kmalloc_dma(unsigned long size)
{
    printf("allocating %x bytes of DMA memory\n", size);
    size = align(size, PAGE_SIZE);

    struct dma_chunk *chunk = find_dma(free_dma, size);

    if (chunk) {
        remove_dma(&free_dma, chunk->virtual);

        struct dma_chunk *new = split_dma(chunk, size);
        insert_dma(&free_dma, new);
        insert_dma(&dma, chunk);

        return (void *)(chunk->virtual);
    }

    int err = dma_alloc_pages(kheap_top, false, true, size / PAGE_SIZE);
    if (err < 0) {
        errno = ENOMEM;
        return NULL;
    }

    unsigned long addr = kheap_top;
    kheap_top += size;

    chunk = kzalloc(MEM_GEN, sizeof(*chunk));
    chunk->size = size;
    chunk->virtual = addr;
    insert_dma(&dma, chunk);

    return (void *)(chunk->virtual);
}

void kfree_dma(void *virtual)
{
    printf("freeing DMA memory at %x\n", virtual);
    struct dma_chunk *chunk = remove_dma(&dma, (unsigned long)virtual);
    if (!chunk) {
        PANIC("Unable to find DMA chunk to free!");
    }

    insert_dma(&free_dma, chunk);
}
