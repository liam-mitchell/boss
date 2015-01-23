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
        /* printf("returning null split size %x chunksize %x\n", size, chunk->size); */
        return NULL;
    }

    unsigned long new_size = chunk->size - sizeof(unsigned long) - size;
    struct chunk *new = (struct chunk *)(USER_PTR(chunk) + size);

    chunk->size = size;

    new->size = new_size;
    new->next = NULL;
    new->prev = NULL;

    /* printf("split chunk %x and %x (sizes %x / %x)\n", chunk, new, chunk->size, new->size); */

    return new;
}

static void defrag_before(struct chunk *chunk)
{
    while (chunk->prev
           && (USER_PTR(chunk->prev) + chunk->prev->size == chunk))
    {
        /* printf("combining chunk %x with %x (sizes %x and %x)\n", chunk, chunk->prev, chunk->size, chunk->prev->size); */
        chunk->prev->size += chunk->size + sizeof(unsigned long);
        chunk->prev->next = chunk->next;

        if (chunk->next) {
            chunk->next->prev = chunk->prev;
        }

        chunk = chunk->prev;

        /* printf("size now %x\n", chunk->size); */
    }
}

static void defrag_after(struct chunk *chunk)
{
    while (chunk->next
           && (USER_PTR(chunk) + chunk->size == chunk->next))
    {
        /* printf("combining chunk %x with %x (sizes %x and %x) ", chunk, chunk->next, chunk->size, chunk->next->size); */

        chunk->size += chunk->next->size + sizeof(unsigned long);
        chunk->next = chunk->next->next;

        if (chunk->next) {
            chunk->next->prev = chunk;
        }

        /* printf("size now %x\n", chunk->size); */
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

    /* printf("added %x to list before %x\n" */
           /* " other chunk prev: %x next %x size %x\n" */
           /* " new chunk prev: %x next %x size %x\n", */
           /* new, chunk, chunk->prev, chunk->next, chunk->size, new->prev, new->next, new->size); */
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

    /* printf("adding %x to list %x (size %x)\n", chunk, list, chunk->size); */

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
    /* printf("kmalloc_general: allocating %x bytes general memory... ", size); */
    struct chunk *curr = find_chunk(free, size);
    if (!curr) {
        /* printf("new chunk\n"); */
        return alloc_chunk(size);
    }

    /* printf("removing chunk %x\n", curr); */
    remove_chunk(&free, curr);

    if (curr->size > size + sizeof(*curr)) {
        struct chunk *new = split_chunk(curr, size);
        add_chunk(&free, new);
    }

    /* printf("kmalloc_general: returning %x (chunk size %x)\n", USER_PTR(curr), curr->size); */
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
    /* printf("kfree: freeing chunk %x (size %x)\n", chunk, chunk->size); */

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

/* static void print_chunk(struct chunk *chunk) */
/* { */
/*     printf("chunk at %x: size %x next %x prev %x\n", chunk, chunk->size, chunk->next, chunk->prev); */
/* } */

/* static void print_chunks() */
/* { */
/*     printf("init_kheap: free chunk list\n"); */
/*     for (struct chunk *curr = free; curr; curr = curr->next) { */
/*         printf("\t"); */
/*         print_chunk(curr); */
/*     } */
/* } */

static void kheap_test()
{
    /* printf("kheap_test: kheap_top = %x\n", kheap_top); */
    /* print_chunks(); */
    /* char *buf1 = kmalloc(MEM_GEN, 12); */
    /* char *buf2 = kmalloc(MEM_GEN, 8); */
    /* char *buf3 = kmalloc(MEM_GEN, 1); */
    /* char *buf4 = kmalloc(MEM_GEN, 0); */
    /* char *buf5 = kmalloc(MEM_GEN, PAGE_SIZE); */
    /* char *buf6 = kmalloc(MEM_GEN, 3); */
    /* char *buf7 = kmalloc(MEM_GEN, 189); */

    /* printf("first time through alloc - "); */
    /* print_chunks(); */
    /* printf("buffers allocated at: %x\n%x\n%x\n%x\n%x\n%x\n%x\n", buf1, buf2, buf3, buf4, buf5, buf6, buf7); */
    /* kfree(buf1); */
    /* kfree(buf2); */

    /* kfree(buf5); */
    /* kfree(buf7); */

    /* printf("after freeing - "); */
    /* print_chunks(); */

    /* buf1 = kmalloc(MEM_GEN, 40); */
    /* buf2 = kmalloc(MEM_GEN, 2); */
    /* buf5 = kmalloc(MEM_GEN, 7); */
    /* kfree(buf5); */
    /* kfree(buf2); */
    /* kfree(buf6); */

    /* printf("more stuff was done - "); */
    /* print_chunks(); */
    /* printf("buffers allocated at: %x\n%x\n%x\n%x\n%x\n%x\n%x\n", buf1, buf2, buf3, buf4, buf5, buf6, buf7); */
    /* kfree(buf1); */
    /* kfree(buf3); */
    /* kfree(buf4); */

    /* printf("all chunks freed - "); */
    /* print_chunks(); */
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

    kheap_test();
}
