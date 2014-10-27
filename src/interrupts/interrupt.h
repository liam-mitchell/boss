#ifndef __INTERRUPT_H_
#define __INTERRUPT_H_

#include <stdint.h>

typedef struct registers_t
{
	uint32_t ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t interrupt, error;
	uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*interrupt_callback)(registers_t *); // allows registration of different callbacks for each interrupt
void register_interrupt_callback(uint8_t n, interrupt_callback cb); // register a function as a callback

#endif
