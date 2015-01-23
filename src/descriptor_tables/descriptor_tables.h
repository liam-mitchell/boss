#ifndef __DESCRIPTOR_TABLES_H_
#define __DESCRIPTOR_TABLES_H_

#include "compiler.h"

#include <stdint.h>

void init_descriptor_tables();
void flush_idt();

void enable_irq(uint8_t irq);
void disable_irq(uint8_t irq);

void set_esp0(uint32_t new);

#endif // __DESCRIPTOR_TABLES_H
