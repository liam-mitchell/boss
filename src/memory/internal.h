#ifndef __KHEAP_INTERNAL_H_
#define __KHEAP_INTERNAL_H_

#define CHUNK_PTR(usr) (struct chunk *)((void *)(usr) - sizeof(unsigned long))
#define USER_PTR(chnk) ((void *)(chnk) + sizeof(unsigned long))

struct chunk {
    unsigned long size;
    struct chunk *prev;
    struct chunk *next;
};

struct dma_chunk {
    unsigned long size;
    unsigned long virtual;
    struct dma_chunk *next;
};

extern unsigned long kheap_top;
extern struct chunk *free;
extern struct dma_chunk *free_dma;
extern struct dma_chunk *dma;

void *kmalloc_dma(unsigned long size);
void kfree_dma(void *address);

#endif // __KHEAP_INTERNAL_H_
