#include "vmm.h"

#include "errno.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
#include "pmm.h"
#include "printf.h"

extern ldsymbol ld_temp_pages;
extern ldsymbol ld_temp_pages_end;

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel);

uint32_t **get_page_directory_entry(uint32_t virtual)
{
    return (uint32_t **)0xFFFFF000 + DIRINDEX(virtual);
}

uint32_t *get_page_table(uint32_t virtual)
{
    return (uint32_t *)(0xFFC00000 + DIRINDEX(virtual) * PAGE_SIZE);
}

uint32_t *get_page(uint32_t virtual)
{
    return (uint32_t *)get_page_table(virtual) + TBLINDEX(virtual);
}

uint32_t map_physical(uint32_t physical)
{
    for (uint32_t virtual = (uint32_t)ld_temp_pages;
         virtual < (uint32_t)ld_temp_pages_end;
         virtual += PAGE_SIZE)
    {
        uint32_t *page = get_page(virtual);
        if (!PG_IS_PRESENT(*page)) {
            if (map_page(virtual, physical, 0, 1) < 0) {
                continue;
            }

            uint32_t offset = physical & 0xFFF;
            return virtual + offset;
        }
    }

    printf("failed to map physical address %x", physical);
    return 0;
}

void flush_tlb(uint32_t virtual)
{
    asm volatile ("invlpg (%0)" : : "r"(virtual) : );
}

void unmap_page(uint32_t virtual)
{
    uint32_t **direntry = get_page_directory_entry(virtual);

    if (!PG_IS_PRESENT((uint32_t)*direntry)) {
        PANIC("Attempted to unmap page with no associated page table!");
    }

    uint32_t *page = get_page(virtual);

    if (!PG_IS_PRESENT(*page)) {
        PANIC("Attempted to unmap a page that wasn't there!");
    }

    *page = 0;
    flush_tlb(virtual);
}

static void unmap_pages(uint32_t virtual, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) {
        unmap_page(virtual + i * PAGE_SIZE);
    }
}

void set_page_attributes(uint32_t *page, bool present,
                         bool writeable, bool user)
{
    if (writeable) *page |= PG_WRITEABLE;
    else *page &= ~PG_WRITEABLE;

    if (user) *page |= PG_USER;
    else *page &= ~PG_USER;

    if (present) *page |= PG_PRESENT;
    else *page &= ~PG_PRESENT;
}

static int map_page(uint32_t virtual, uint32_t physical,
                    uint8_t readonly, uint8_t kernel)
{
    uint32_t **direntry = get_page_directory_entry(virtual);

    if (!PG_IS_PRESENT((uint32_t)*direntry)) {
        uint32_t frame = alloc_frame();
        if (frame == 0) {
            return -ENOMEM;
        }

        *direntry =
            (uint32_t *)((frame & 0xFFFFF000)
                         | PG_USER
                         | PG_PRESENT
                         | PG_WRITEABLE);

        printf("Allocated a new directory entry\n");

        memset(get_page_table(virtual), 0, PAGE_SIZE);
    }

    uint32_t *page = get_page(virtual);

    if (PG_IS_PRESENT(*page)) {
        return -EINVAL;
    }

    /* *page |= PG_PRESENT; */

    /* if (!readonly) { */
    /*     *page |= PG_WRITEABLE; */
    /* } */

    /* if (!kernel) { */
    /*     *page |= PG_USER; */
    /* } */

    set_page_attributes(page, true, !readonly, !kernel);
    *page |= physical & 0xFFFFF000;

    flush_tlb(virtual);
    return 1;
}

int alloc_page(uint32_t virtual, uint8_t readonly, uint8_t kernel)
{
    uint32_t physical = alloc_frame();

    if (!physical) {
        return -ENOMEM;
    }

    map_page(virtual, physical, readonly, kernel);
    
    return 1;
}

int alloc_pages(uint32_t virtual, uint8_t readonly,
                uint8_t kernel, uint32_t num)
{
    uint32_t start = virtual;
    uint32_t end = virtual + num * PAGE_SIZE;

    int err = 0;
    while (virtual < end) {
        err = alloc_page(virtual, readonly, kernel);
        if (err < 0) {
            goto free_pages;
        }

        virtual += PAGE_SIZE;
    }

    return err;

 free_pages:
    while (virtual > start) {
        virtual -= PAGE_SIZE;
        free_page(virtual);
    }

    return err;
}

int dma_alloc_pages(uint32_t virtual, bool readonly, bool kernel, uint32_t n)
{
    uint32_t physical = dma_alloc_frames(n);
    if (!physical) {
        return -ENOMEM;
    }
    
    for (uint32_t i = 0; i < n; ++i) {
        map_page(virtual, physical, readonly, kernel);
        virtual += PAGE_SIZE;
        physical += PAGE_SIZE;
    }

    return 1;
}

void dma_free_pages(uint32_t virtual, uint32_t n)
{
    dma_free_frames(get_physical(virtual), n);
    unmap_pages(virtual, n);
}

void free_page(uint32_t virtual)
{
    free_frame(virtual);
    unmap_page(virtual);
}

bool check_user_ptr(void __user *ptr)
{
    uint32_t *pde = (uint32_t *)get_page_directory_entry((uint32_t)ptr);
    if (!PG_IS_PRESENT(*pde)) {
        return false;
    }

    uint32_t *page = get_page((uint32_t)ptr);
    if (!PG_IS_PRESENT(*page) || !PG_IS_USERMODE(*page)) {
        return false;
    }

    return true;
}

uint32_t get_physical(uint32_t virtual)
{
    uint32_t *pde = (uint32_t *)get_page_directory_entry((uint32_t)virtual);
    if (!PG_IS_PRESENT(*pde)) {
        return 0;
    }

    uint32_t *page = get_page((uint32_t)virtual);
    if (!PG_IS_PRESENT(*page)) {
        return 0;
    }

    return PG_FRAME(*page);
}
