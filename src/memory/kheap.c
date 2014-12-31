#include "kheap.h"

#include "errno.h"
#include "ldsymbol.h"
#include "macros.h"
#include "mm.h"
#include "memory.h"

typedef struct chunk {
    uint32_t size;
    struct chunk *prev;
    struct chunk *next;
} chunk_t;

#define CHUNK_PTR(usr) ((void *)(usr) - sizeof(chunk_t))
#define USER_PTR(chnk) ((void *)(chnk) + sizeof(chunk_t))

extern ldsymbol ld_heap_start;
static uint32_t kheap_top;
static chunk_t *first_free_chunk;

static void print_chunk(chunk_t *chunk)
{
    puts("@");
    puth((uint32_t)chunk);
    puts(": ");
    puth(chunk->size);
    puts("[->");
    puth((uint32_t)chunk->next);
    puts("][<-");
    puth((uint32_t)chunk->prev);
    puts("]\n");
}

void print_chunk_list(void)
{
    puts("FREE CHUNK LIST:\n");
    for (chunk_t *chunk = first_free_chunk;
         chunk != NULL;
         chunk = chunk->next)
    {
        puts("chunk: ");
        print_chunk(chunk);
    }
}

static uint32_t next_page(uint32_t address)
{
    if (address & 0x00000FFF) {
        address &= 0xFFFFF000;
        address += PAGE_SIZE;
    }

    return address;
}

static chunk_t *split_chunk(chunk_t *chunk, uint32_t size)
{
    if (size + sizeof(*chunk) >= chunk->size) {
        PANIC("Tried to split a chunk into larger parts than the whole!");
    }

    chunk_t *new = USER_PTR(chunk) + size;
    new->size = chunk->size - sizeof(*chunk) - size;
    chunk->size = size;

    return new;
}

static void combine_chunks(chunk_t *first, chunk_t *second)
{
    first->size += second->size + sizeof(*second);
}

static uint8_t can_combine_chunks(chunk_t *first, chunk_t *second)
{
    if (USER_PTR(first) + first->size == (void *)second) {
        return 1;
    }

    return 0;
}

static void defrag_after(chunk_t *chunk) {
    while (can_combine_chunks(chunk, chunk->next)) {
        combine_chunks(chunk, chunk->next);

        chunk->next->prev = chunk;
        chunk->next = chunk->next->next;
    }
}

static void insert_after(chunk_t *chunk, chunk_t *new) {
    if (chunk->next) {
        chunk->next->prev = new;
    }

    new->next = chunk->next;
    new->prev = chunk;
    chunk->next = new;

    defrag_after(chunk);
}

static void insert_before(chunk_t *chunk, chunk_t *new) {
    if (chunk == first_free_chunk) {
        first_free_chunk = new;
    }

    if (chunk->prev) {
        chunk->prev->next = new;
    }
    
    new->prev = chunk->prev;
    new->next = chunk;
    chunk->prev = new;

    defrag_after(new);
}

static chunk_t *remove_chunk(chunk_t *chunk) {
    if (chunk->next) {
        chunk->next->prev = chunk->prev;
    }
    if (chunk->prev) {
        chunk->prev->next = chunk->next;
    }
    if (chunk == first_free_chunk) {
        first_free_chunk = chunk->next;
    }

    return chunk;
}

static void insert_chunk(chunk_t *chunk)
{
    chunk_t *free = first_free_chunk;

    if (free == NULL) {
        chunk->next = NULL;
        chunk->prev = NULL;
        first_free_chunk = chunk;
    }
    else {
        while (free != NULL) {
            if (free >= chunk) {
                insert_before(free, chunk);
                break;
            }
            if (free->next == NULL) {
                insert_after(free, chunk);
                break;
            }

            free = free->next;
        }
    }
}

static void *kmalloc_large(uint32_t size)
{
    uint32_t alloc_size = size + sizeof(chunk_t);
    uint32_t num_pages;
    uint32_t aligned_size = next_page(alloc_size);
    
    num_pages = aligned_size / PAGE_SIZE;

    int err = alloc_pages(kheap_top, 0, 1, num_pages);
    if (err < 0) {
        errno = ENOMEM;
        return NULL;
    }

    chunk_t *chunk = (chunk_t *)kheap_top;
    chunk->size = aligned_size - sizeof(chunk_t);
    chunk->next = NULL;
    chunk->prev = NULL;

    if (size < chunk->size - sizeof(chunk_t)) {
        chunk_t *new = split_chunk(chunk, size);
        insert_chunk(new);
    }
    
    return USER_PTR(chunk);
}

void *kmalloc(uint32_t size)
{
    if (size & 3) {
        size &= 0xFFFFFFFC;
        size += 4;
    }

    if (size > PAGE_SIZE - sizeof(chunk_t)) {
        return kmalloc_large(size);
    }

    chunk_t *chunk = first_free_chunk;

    while (chunk != NULL) {
        if (chunk->size == size) {
            return USER_PTR(remove_chunk(chunk));
        }
        if (chunk->size > size + sizeof(*chunk)) {
            remove_chunk(chunk);
            chunk_t *new = split_chunk(chunk, size);
            insert_chunk(new);
            return USER_PTR(chunk);
        }

        chunk = chunk->next;
    }

    int err = alloc_page(kheap_top, 0, 1);
    if (err < 0) {
        errno = ENOMEM;
        return NULL;
    }

    chunk_t *ret = (chunk_t *)kheap_top;
    ret->size = PAGE_SIZE - sizeof(*ret);
    
    kheap_top += PAGE_SIZE;

    chunk_t *new = split_chunk(ret, size);
    insert_chunk(new);
    return USER_PTR(ret);
}

void *kcalloc(uint32_t num, uint32_t size)
{
    void *ret = kmalloc(size * num);
    memset(ret, 0, size * num);
    return ret;
}

void *kzalloc(uint32_t size)
{
    return kcalloc(1, size);
}

void kfree(void *address)
{
    if (address == NULL) {
        return;
    }
    
    chunk_t *chunk = CHUNK_PTR(address);
    chunk->next = NULL;
    chunk->prev = NULL;
    insert_chunk(chunk);
}

void init_kheap(void)
{
    puts("Initializing kernel heap...\n");
    kheap_top = next_page((uint32_t)ld_heap_start);
    
    int err = alloc_page(kheap_top, 0, 1);
    if (err < 0) {
        PANIC("Unable to allocate pages for the kernel heap!");
    }

    chunk_t *first_chunk = (chunk_t *)kheap_top;
    first_chunk->size = PAGE_SIZE - sizeof(*first_chunk);
    insert_chunk(first_chunk);
    
    kheap_top += PAGE_SIZE;
    puts("Initialized kernel heap!\n");
}
