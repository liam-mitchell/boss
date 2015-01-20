#include "pmm.h"

#include "bits.h"
#include "descriptor_tables.h"
#include "errno.h"
#include "interrupt.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
#include "mboot.h"
#include "printf.h"
#include "task.h"
#include "terminal.h"
#include "vmm.h"

#include <stdint.h>

#define for_each_mmap_entry(entry, end, mboot)                                  \
    for (entry = (memory_map_t *)(mboot->mmap_addr),                            \
                       end = entry + mboot_info->mmap_length;                   \
    entry < end;                                                                \
    entry = (memory_map_t *)((unsigned int)entry                                \
                             + entry->size                                      \
                             + sizeof(unsigned long)))

#define MEM_USABLE 1
#define MEM_RESERVED 2
#define MEM_RECLAIMABLE 3
#define MEM_NVS 4
#define MEM_BAD 5

#define PMM_DMA_SIZE 1 * MBYTE
#define PMM_DMA_NPAGES (PMM_DMA_SIZE / PAGE_SIZE)

extern ldsymbol ld_virtual_offset;
extern ldsymbol ld_virtual_end;
extern ldsymbol ld_physical_end;

static uint32_t free_stack_top;
static uint32_t dma_bitmap[PMM_DMA_NPAGES / 32];

static uint32_t dma_start()
{
    return align((uint32_t)ld_physical_end, PAGE_SIZE);
}

static uint32_t dma_end()
{
    return dma_start() + PMM_DMA_SIZE;
}

static uint32_t dma_frame(uint32_t physical)
{
    return (physical - dma_start()) >> 12;
}

static uint32_t dma_index(uint32_t physical)
{
    return dma_frame(physical) / 32;
}

static uint32_t dma_bit(uint32_t physical)
{
    return dma_frame(physical) % 32;
}

static void dma_set_frame(uint32_t physical, bool allocated)
{
    if (!check_dma_address(physical)) {
        PANIC("Attempted to allocate a frame for DMA outside the allowed range");
    }

    uint32_t index = dma_index(physical);
    uint32_t bit = dma_bit(physical);

    if (allocated) {
        SET_BIT(dma_bitmap[index], bit);
    }
    else {
        CLR_BIT(dma_bitmap[index], bit);
    }
}

static void dma_set_frames(uint32_t physical, uint32_t n, bool allocated)
{
    for (uint32_t i = 0; i < n; ++i) {
        dma_set_frame(physical + i * PAGE_SIZE, allocated);
    }
}

static bool dma_frame_is_free(uint32_t physical)
{
    if (!check_dma_address(physical)) {
        printf("Address was not valid DMA address: %x\n", physical);
        return false;
    }

    uint32_t index = dma_index(physical);
    uint32_t bit = dma_bit(physical);
    printf("Address was valid DMA address: bitmap at index %x (checking bit %d)\n", dma_bitmap[index], bit);

    return !TST_BIT(dma_bitmap[index], bit);
}

static bool dma_frames_are_free(uint32_t physical, uint32_t n)
{
    printf("Checking if %x DMA frames are free at %x\n", n, physical);
    for (uint32_t i = 0; i < n; ++i) {
        if (!dma_frame_is_free(physical + i * PAGE_SIZE)) {
            printf("Frame %x was not free\n", i);
            return false;
        }
    }

    return true;
}

uint32_t dma_alloc_frames(uint32_t n)
{
    for (uint32_t physical = dma_start();
         physical < dma_end();
         physical += PAGE_SIZE)
    {
        printf("Checking DMA address %x\n", physical);
        if (dma_frames_are_free(physical, n)) {
            dma_set_frames(physical, n, true);
            return physical;
        }
    }

    return 0;
}

void dma_free_frames(uint32_t physical, uint32_t n)
{
    dma_set_frames(physical, n, false);
}

uint32_t _alloc_frame()
{
    uint32_t ret = free_stack_top;

    printf("allocating frame %x (free_stack_top %x)\n", ret, free_stack_top);
    uint32_t virtual = map_physical(ret);
    memcpy(&free_stack_top, (void *)virtual, sizeof(free_stack_top));
    unmap_page(virtual);
    printf("free stack top now %x\n", free_stack_top);
    while(1);

    return ret;
}

uint32_t alloc_frame()
{
    uint32_t ret = free_stack_top;

    uint32_t virtual = map_physical(ret);
    memcpy(&free_stack_top, (void *)virtual, sizeof(free_stack_top));
    unmap_page(virtual);

    return ret;
}

void free_frame(uint32_t virtual)
{
    memcpy((void *)virtual, &free_stack_top, sizeof(free_stack_top));
    free_stack_top = get_physical(virtual);
}

static inline
int mem_range_is_free(multiboot_info_t *mboot_info,
                      uint32_t start, uint32_t end)
{
    if (mboot_info->flags & (1 << 6)) {
        memory_map_t *entry;
        memory_map_t *last_entry;

        for_each_mmap_entry(entry, last_entry, mboot_info) {
            uint32_t entry_start = entry->base_addr_low;
            uint32_t entry_end = entry_start + entry->length_low;

            if (entry_end < start) {
                continue;
            }

            if (entry_start < start) {
                if (entry_end > end) {
                    return 1;
                }
                else {
                    start = entry_end;
                }
            }
            if (start > end) {
                return 0;
            }
        }
    }

    return 0;
}

static uint32_t init_pmm_dma(multiboot_info_t *mboot)
{
    uint32_t *bitmap = dma_bitmap - ((uint32_t)ld_virtual_offset / 4);
    uint32_t count = 0;

    for (uint32_t address = dma_start();
         address < dma_end();
         address += PAGE_SIZE)
    {
        uint32_t index = dma_index(address);
        uint32_t bit = dma_bit(address);

        if (mem_range_is_free(mboot, address, address + PAGE_SIZE)) {
            CLR_BIT(bitmap[index], bit);
            ++count;
        }
        else {
            SET_BIT(bitmap[index], bit);
        }
    }

    return count;
}

static uint32_t init_pmm_general(multiboot_info_t *mboot)
{
    uint32_t *stack = &free_stack_top;
    stack -= ((uint32_t)ld_virtual_offset / sizeof(uint32_t));
    *stack = 0;

    uint32_t count = 0;

    for (uint32_t address = dma_end();
         address < 0xFFFFF000;
         address += PAGE_SIZE)
    {
        if (mem_range_is_free(mboot, address, address + PAGE_SIZE)) {
            *(uint32_t *)address = *stack;
            *stack = address;
            ++count;
        }
    }

    return count;
}

/**
 * Initialize the free frame stack by embedding a pointer to the next
 * free frame in the first word of of each free frame
 *
 * This requires the virtual memory manager to return us each free frame pointer
 * once it maps the frame into virtual memory - however, it requires only four
 * bytes of memory for the physical memory manager total!
 */
uint32_t init_pmm(multiboot_info_t *mboot)
{
    if (!(mboot->flags & (1 << 6))) {
        return 0;
    }

    uint32_t dma_frames = init_pmm_dma(mboot);
    uint32_t gen_frames = init_pmm_general(mboot);

    return dma_frames + gen_frames;
}

bool check_dma_address(uint32_t physical)
{
    if (physical < dma_start() || physical > dma_end()) {
        return false;
    }

    return true;
}

static void page_fault_handler(registers_t *regs)
{
    uint32_t address;
    asm volatile ("mov %%cr2, %0" : "=r"(address) : : );

    if (current_task) {
        printf("[PAGE FAULT] current_task: %d\n", current_task->pid);
    }

    printf("[PAGE FAULT] address: %x (%s %s %s page)\n", address,
           (regs->error & 0x4) ? "user" : "kernel",
           (regs->error & 0x2) ? "write to" : "read from",
           (regs->error & 0x1) ? "present" : "not present");

    printf("registers:\n"
           " eip: %x\n"
           " esp: %x\n"
           " cs:  %x\n"
           " ss:  %x\n"
           " eax: %x\n"
           " ecx: %x\n"
           " edx: %x\n"
           " ebp: %x\n"
           " esi: %x\n"
           " edi: %x\n",
           regs->eip,
           regs->esp,
           regs->cs,
           regs->ss,
           regs->eax,
           regs->ecx,
           regs->edx,
           regs->ebx,
           regs->esi,
           regs->edi);

    if (!(regs->error & 0x4)) {
        PANIC("Kernel page fault!");
    }
}

void init_paging()
{
    register_interrupt_callback(0x0E, &page_fault_handler);
}
