#include "memory/address-space.h"

#include "algorithm.h"
#include "errno.h"
#include "memory/kheap.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory/memory.h"
#include "task.h"
#include "memory/pmm.h"
#include "printf.h"
#include "memory/vmm.h"

extern ldsymbol ld_virtual_offset;

address_space_t *alloc_address_space()
{
    address_space_t *as = kzalloc(MEM_GEN, sizeof(*as));
    as->pgdir = kzalloc(MEM_GEN, PAGE_SIZE);

    return as;
}

void free_address_space(address_space_t *as)
{
    if (as) {
        kfree(as->pgdir);
        kfree(as);
    }
}

static uint32_t clone_page(uint32_t virtual)
{
    uint32_t **pde = get_page_directory_entry(virtual);
    if (!PG_IS_PRESENT((uint32_t)*pde)) {
        return 0;
    }

    uint32_t *page = get_page(virtual);
    if (!PG_IS_USERMODE(*page)) {
        return *page;
    }

    uint32_t new = alloc_frame();

    uint32_t vnew = map_physical(new);
    memcpy((void *)vnew, (void *)PG_FRAME(virtual), PAGE_SIZE);
    unmap_page(vnew);

    new |= PG_INFO(*page);

    return new;
}

static uint32_t *clone_page_table(uint32_t virtual)
{
    uint32_t **pde = get_page_directory_entry(virtual);

    if (!PG_IS_PRESENT((uint32_t)*pde)) {
        return NULL;
    }

    uint32_t new = alloc_frame();
    uint32_t *vnew = (uint32_t *)map_physical(new);


    for (uint32_t i = 0; i < PAGE_SIZE / 4; ++i) {
        vnew[i] = clone_page(virtual + i * PAGE_SIZE);
    }

    unmap_page((uint32_t)vnew);
    new |= PG_INFO((uint32_t)*pde);

    return (uint32_t *)new;
}

static uint32_t **clone_page_directory()
{
    uint32_t **pgdir = kzalloc(MEM_GEN, PAGE_SIZE);
    if (!pgdir) {
        return NULL;
    }
    
    for (uint32_t virtual = 0;
         virtual < (uint32_t)ld_virtual_offset;
         virtual += PAGE_SIZE * PAGE_SIZE / 4)
    {
        /* printf("cloning page table %x\n", virtual); */
        pgdir[DIRINDEX(virtual)] = clone_page_table(virtual);
    }

    return pgdir;
}

address_space_t *clone_address_space()
{
    /* printf("cloning address space...\n"); */
    address_space_t *as = kzalloc(MEM_GEN, sizeof(*as));
    if (!as) {
        return NULL;
    }

    /* printf("cloning page directory...\n"); */
    as->pgdir = clone_page_directory();
    if (!as->pgdir) {
        kfree(as);
        return NULL;
    }

    as->brk = current_task->as->brk;
    return as;
}


void save_address_space(address_space_t *as)
{
    if (!as) {
        return;
    }

    uint32_t virtual = 0;
    do {
        uint32_t **pde = get_page_directory_entry(virtual);
        as->pgdir[DIRINDEX(virtual)] = *pde;

        virtual += 0x400000;
    } while (virtual > 0);
}

static void switch_page_table(const address_space_t *as, uint32_t virtual)
{
    uint32_t **pde = get_page_directory_entry(virtual);
    uint32_t **new_pde = &as->pgdir[DIRINDEX(virtual)];

    if (!PG_FRAME((uint32_t)*pde != PG_FRAME((uint32_t)*new_pde))) {
        /* printf("switching page table %x to %x\n", *pde, *new_pde); */

        *pde = *new_pde;

        for (uint32_t i = 0; i < PAGE_SIZE / 4; ++i) {
            flush_tlb(virtual + i * PAGE_SIZE);
        }
    }
}

void switch_address_space(address_space_t *old, const address_space_t *new)
{
    if (old == new) {
        return;
    }

    save_address_space(old);

    for (uint32_t virtual = 0;
         virtual < (uint32_t)ld_virtual_offset;
         virtual += 0x400000)
    {
        switch_page_table(new, virtual);
    }
}

static int as_alloc_page(address_space_t *as, uint32_t virtual,
                         bool readonly, bool kernel)
{
    uint32_t *pde = (uint32_t *)&as->pgdir[DIRINDEX(virtual)];
    if (!PG_IS_PRESENT(*pde)) {
        uint32_t frame = alloc_frame();
        if (!frame) {
            return -ENOMEM;
        }

        *pde = frame | PG_USER | PG_WRITEABLE | PG_PRESENT;
    }

    uint32_t *pt = (uint32_t *)(map_physical(*pde) & ~0xFFF);
    uint32_t *page = &pt[TBLINDEX(virtual)];

    *page = alloc_frame();
    if (!*page) {
        free_frame(PG_FRAME(*pde));
        return -ENOMEM;
    }

    set_page_attributes(page, true, !readonly, !kernel);

    unmap_page((uint32_t)pt);

    return 0;
}

static void as_free_page(address_space_t *as, uint32_t virtual)
{
    /* TODO: free the page tables too if we can... */
    uint32_t *pde = (uint32_t *)&as->pgdir[DIRINDEX(virtual)];
    if (!PG_IS_PRESENT(*pde)) {
        return;
    }

    uint32_t *pt = (uint32_t *)map_physical(PG_FRAME(*pde));
    uint32_t *page = &pt[TBLINDEX(virtual)];

    if (*page) {
        free_frame(PG_FRAME(*page));
    }

    unmap_page((uint32_t)pt);
}

int map_as_data(address_space_t *as, uint32_t len, void *data)
{
    void *start = data;
    int err = 0;
    for (uint32_t virtual = 0; virtual < len; virtual += PAGE_SIZE) {
        err = as_alloc_page(as, virtual, false, false);
        if (err < 0) {
            goto free_pages;
        }

        uint32_t *pt =
            (uint32_t *)map_physical(PG_FRAME((uint32_t)as->pgdir[DIRINDEX(virtual)]));
        uint32_t *page =
            (uint32_t *)map_physical(PG_FRAME(pt[TBLINDEX(virtual)]));

        memcpy(page, data, min(len - virtual, (uint32_t)PAGE_SIZE));
        data += PAGE_SIZE;

        unmap_page((uint32_t)page);
        unmap_page((uint32_t)pt);
    }

    as->brk = align(len, PAGE_SIZE);
    return 0;

 free_pages:
    for (uint32_t virtual = data - start;
         virtual <= (uint32_t)data;
         virtual -= PAGE_SIZE)
    {
        as_free_page(as, virtual);
    }

    return err;
}

int map_as_stack(address_space_t *as)
{
    return as_alloc_page(as, (uint32_t)ld_virtual_offset - 4, false, false);
}
