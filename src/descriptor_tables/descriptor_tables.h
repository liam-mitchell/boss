#ifndef __DESCRIPTOR_TABLES_H_
#define __DESCRIPTOR_TABLES_H_

#include <stdint.h>

struct gdt_ptr_t {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

typedef struct gdt_ptr_t gdt_ptr_t;

struct gdt_entry_t {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed));

typedef struct gdt_entry_t gdt_entry_t;

struct idt_ptr_t {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

typedef struct idt_ptr_t idt_ptr_t;

struct idt_entry_t {
	uint16_t base_low;
	uint16_t segment;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t base_high;
} __attribute__((packed));

typedef struct idt_entry_t idt_entry_t;

void init_descriptor_tables();

#endif // __DESCRIPTOR_TABLES_H
