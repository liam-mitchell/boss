#include "kheap.h"
#include "macros.h"
#include "mm.h"
#include "memory.h"

typedef struct chunk {
    uint32_t size;
    struct chunk *next;
} chunk_t;

#define CHUNK_PTR(usr) ((void *)(usr) - sizeof(chunk_t))
#define USER_PTR(chnk) ((void *)(chnk) + sizeof(chunk_t))

extern uint32_t kernel_virtual_end;
static uint32_t kheap_top;
static chunk_t *first_free_chunk;

static void write_chunk_data(chunk_t *address, uint32_t size, chunk_t *next)
{
    memcpy(address, &size, sizeof(size));
    memcpy(address + sizeof(size), &next, sizeof(next));
}

static chunk_t *split_chunk(chunk_t *chunk, uint32_t size)
{
    if (size > chunk->size - sizeof(*chunk)) {
        PANIC("Tried to split a chunk into larger parts than the whole!");
    }

    chunk_t *new = USER_PTR(chunk) + size;
    
    new->size = chunk->size - size - sizeof(*chunk);
    new->next = chunk->next;

    chunk->next = new;
    chunk->size = size;

    return new;
}

static uint32_t next_page(uint32_t address)
{
    if (address & 0x00000FFF) {
        address &= 0xFFFFF000;
        address += PAGE_SIZE;
    }

    return address;
}

void *kmalloc(uint32_t size)
{
    // Align everything on 4-byte boundaries
    if (size & 4) {
        size &= 0xFFFFFFFC;
        size += 4;
    }
    
    chunk_t *chunk = first_free_chunk;

    if (!chunk) {
        int err = alloc_page(kheap_top, 0, 1);
        if (err < 0) {
            return NULL;
        }

        first_free_chunk = (chunk_t *)kheap_top;
        chunk = first_free_chunk;
        kheap_top += PAGE_SIZE;
    }

    if (chunk->size > size) {
        first_free_chunk = split_chunk(chunk, size);
        return USER_PTR(chunk);
    }
    if (chunk->size == size) {
        first_free_chunk = chunk->next;
        return USER_PTR(chunk);
    }
    
    chunk_t *next;
    for (next = chunk->next;
         next != NULL;
         chunk = next, next = next->next)
    {
        if (next->size > size) {
            chunk_t *new = split_chunk(next, size);
            chunk->next = new;
            return USER_PTR(next);
        }
        if (next->size == size) {
            chunk->next = next->next;
            return USER_PTR(next);
        }
    }

    int err = alloc_page(kheap_top, 0, 1);
    if (err < 0) {
        return NULL;
    }

    next = (chunk_t *)kheap_top;
    chunk->next = split_chunk(next, size);
    kheap_top += PAGE_SIZE;

    return USER_PTR(next);
}

void kfree(void *address)
{
    chunk_t *chunk = CHUNK_PTR(address);
    puts("freeing address: ");
    puth((uint32_t)address);
    putc('\n');

    puts("chunk: ");
    puth((uint32_t)chunk);
    putc('\n');

    puts("size ");
    puth(chunk->size);
    puts(" next: ");
    puth((uint32_t)chunk->next);
    putc('\n');
    
    chunk->next = first_free_chunk;
    first_free_chunk = chunk;
}

void init_kheap()
{
    puts("Initializing kernel heap...\n");
    kheap_top = next_page((uint32_t)&kernel_virtual_end);
    puts("kheap_top: ");
    puth(kheap_top);
    putc('\n');
    int err = alloc_page(kheap_top, 0, 1);
    if (err < 0) {
        PANIC("Unable to allocate pages for the kernel heap!");
    }
    
    puts("allocated page!\n");
    first_free_chunk = (chunk_t *)kheap_top;
    write_chunk_data(first_free_chunk, PAGE_SIZE - sizeof(chunk_t), NULL);
    puts("wrote chunk data to first_free_chunk\n");
}
