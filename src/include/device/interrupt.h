#ifndef __INTERRUPT_H_
#define __INTERRUPT_H_

#include <stdint.h>


#include "memory/pmm.h"

typedef struct registers
{
    uint32_t ds;
    uint32_t edi, esi, ebp, unused, ebx, edx, ecx, eax;
    uint32_t interrupt, error;
    uint32_t eip, cs, eflags, esp, ss;
} registers_t;

typedef void (*interrupt_handler)(registers_t *);
void register_interrupt_handler(uint8_t n, interrupt_handler cb); // register a function as a callback

#endif
