#include "descriptor_tables.h"
#include "memory.h"
#include "terminal.h"
#include "device_io.h"

#define PIC1_C 0x20
#define PIC1_D 0x21
#define PIC2_C 0xA0
#define PIC2_D 0xA1

extern void gdt_flush(uint32_t ptr);
extern void idt_flush(uint32_t ptr);

static void gdt_init();
static void gdt_set_gate(int32_t entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);

static void idt_init();
static void idt_set_gate(uint8_t entry, uint32_t base, uint32_t segment, uint8_t type_attr);

static void pic_remap(int master, int slave);

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

gdt_entry_t gdt_entries[5];
idt_entry_t idt_entries[256];
gdt_ptr_t gdt_ptr;
idt_ptr_t idt_ptr;

void init_descriptor_tables() 
{
	puts("Initializing descriptor tables...\n");
	gdt_init();
	idt_init();
}

static void gdt_init() 
{
	puts("Initializing GDT...\n");
	gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
	gdt_ptr.base = (uint32_t)&gdt_entries;
	gdt_set_gate(0, 0, 0, 0, 0);
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // CS
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // DS
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // userland CS
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // userland DS

	gdt_flush((uint32_t)&gdt_ptr);
	puts("GDT flushed.\n");
}

static void idt_init()
{
	puts("Initializing IDT...\n");
	idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
	idt_ptr.base = (uint32_t)&idt_entries;
	memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);
	puts("IDT initialized at: ");
	puth((uint32_t)&idt_entries);
	putc('\n');

	pic_remap(0x20, 0x28);

	idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
	idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
	idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
	idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
	idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
	idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
	idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
	idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
	idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
	idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
	idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
	idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
	idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
	idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
	idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
	idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
	idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
	idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
	idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
	idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
	idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
	idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
	idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
	idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
	idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
	idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
	idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
	idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
	idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
	idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
	idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
	idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
	idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
	idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
	idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
	idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
	idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
	idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
	idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
	idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
	idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
	idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
	idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
	idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
	idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
	idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
	idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
	idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

	idt_flush((uint32_t)&idt_ptr);
	puts("IDT flushed.\n");
}

static void gdt_set_gate(int32_t entry, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
	gdt_entries[entry].base_low = base & 0xFFFF; // lower 16 bits of base
	gdt_entries[entry].base_middle = (base >> 16) & 0xFF; // middle 8 bits
	gdt_entries[entry].base_high = (base >> 24) & 0xFF;

	gdt_entries[entry].limit_low = limit & 0xFFFF;
	gdt_entries[entry].granularity = (limit >> 16) & 0x0F;
	gdt_entries[entry].granularity |= granularity & 0xF0;
	gdt_entries[entry].access = access;
}

static void idt_set_gate(uint8_t entry, uint32_t base, uint32_t segment, uint8_t type_attr)
{
	idt_entries[entry].base_low = base & 0xFFFF;
	idt_entries[entry].base_high = (base >> 16) & 0xFFFF;

	idt_entries[entry].segment = segment;
	idt_entries[entry].type_attr = type_attr;
	idt_entries[entry].zero = 0;
}

static void pic_remap(int master, int slave)
{
	uint8_t mask1, mask2;
	mask1 = inb(PIC1_D); // save interrupt masks
	mask2 = inb(PIC2_D);

	outb(PIC1_C, 0x11); // start initialization sequence
	io_wait();
	outb(PIC2_C, 0x11);
	io_wait();
	outb(PIC1_D, master); // vector offsets (new ISR numbers)
	io_wait();
	outb(PIC2_D, slave);
	io_wait();
	outb(PIC1_D, 0x04); // daisy-chaining IRQ locations
	io_wait();
	outb(PIC2_D, 0x02);
	io_wait();
	outb(PIC1_D, 0x01); // 8088 mode
	io_wait();
	outb(PIC2_D, 0x01);
	io_wait();

	outb(PIC1_D, mask1);
	outb(PIC2_D, mask2);

	puts("PIC remapped.\n");
}
