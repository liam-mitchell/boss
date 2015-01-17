#include "address-space.h"

#include "kheap.h"
#include "ldsymbol.h"
#include "memory.h"
#include "task.h"
#include "pmm.h"
#include "printf.h"
#include "vmm.h"

extern ldsymbol ld_virtual_offset;

address_space_t *alloc_address_space()
{
    address_space_t *as = kzalloc(MEM_GEN, sizeof(*as));
    as->pgdir = kzalloc(MEM_GEN, PAGE_SIZE);

    return as;
}

void free_address_space(address_space_t *as)
{
    kfree(as->pgdir);
    kfree(as);
}

static uint32_t clone_page(uint32_t virtual)
{
    uint32_t **pde = get_page_directory_entry(virtual);
    if (!PG_IS_PRESENT((uint32_t)*pde)) {
        return 0;
    }

    uint32_t *page = get_page(virtual);
    if (!PG_IS_USERMODE(*page)) {
        /* printf(" Cloning kernel page at vaddr %x: returning %x\n", virtual, *page); */
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
    /* printf("Cloning page table %x (virtual %x pt %x)...\n", *pde, virtual, pde); */

    if (!PG_IS_PRESENT((uint32_t)*pde)) {
        return NULL;
    }

    uint32_t new = alloc_frame();
    uint32_t *vnew = (uint32_t *)map_physical(new);

    /* printf(" User page table, cloning (vnew %x new %x)\n", vnew, new); */

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
    
    for (uint32_t virtual = 0;
         virtual < (uint32_t)ld_virtual_offset;
         virtual += PAGE_SIZE * PAGE_SIZE / 4)
    {
        pgdir[DIRINDEX(virtual)] = clone_page_table(virtual);
    }

    return pgdir;
}

address_space_t *clone_address_space()
{
    address_space_t *as = kzalloc(MEM_GEN, sizeof(*as));

    as->pgdir = clone_page_directory();
    as->brk = run_queue->as->brk;
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
        *pde = *new_pde;

        for (uint32_t i = 0; i < PAGE_SIZE / 4; ++i) {
            flush_tlb(virtual + i * PAGE_SIZE);
        }
    }
}

void switch_address_space(address_space_t *old, const address_space_t *new)
{
    asm volatile ("cli");

    save_address_space(old);

    for (uint32_t virtual = 0;
         virtual < (uint32_t)ld_virtual_offset;
         virtual += 0x400000)
    {
        switch_page_table(new, virtual);
    }

    asm volatile ("sti");
}
