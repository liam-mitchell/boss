#include "descriptor_tables.h"

#include "bits.h"
#include "interrupt.h"
#include "ldsymbol.h"
#include "memory.h"
#include "printf.h"
#include "terminal.h"
#include "port.h"

#define PIC1_C 0x20
#define PIC1_D 0x21
#define PIC2_C 0xA0
#define PIC2_D 0xA1

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __packed;

struct gdt_entry {
    uint32_t low;
    uint32_t high;
};

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __packed;

struct idt_entry {
    uint16_t base_low;
    uint16_t segment;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t base_high;
};

struct tss {
    uint16_t link;
    uint16_t unused1;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t unused2;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t unused3;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t unused4;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t unused5;
    uint16_t cs;
    uint16_t unused6;
    uint16_t ss;
    uint16_t unused7;
    uint16_t ds;
    uint16_t unused8;
    uint16_t fs;
    uint16_t unused9;
    uint16_t gs;
    uint16_t unused10;
    uint16_t ldtr;
    uint16_t unused11;
    uint16_t unused12;
    uint16_t iopb_offset;
};

static volatile struct tss tss;

extern void gdt_flush(uint32_t ptr);
extern void idt_flush(uint32_t ptr);

static void gdt_init();
static void gdt_set_gate(uint32_t entry, uint32_t base,
                         uint32_t limit, uint8_t access);

static void idt_init();
static void idt_set_gate(uint8_t entry, uint32_t base,
                         uint32_t segment, uint8_t type_attr);

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
extern void isr128();

extern ldsymbol ld_tss_stack;

struct gdt_entry gdt_entries[6];
struct idt_entry idt_entries[256];
struct gdt_ptr gdt_ptr;
struct idt_ptr idt_ptr;

void init_descriptor_tables() 
{
    puts("Initializing descriptor tables...\n");
    gdt_init();
    idt_init();
}

/**
 * Load the TSS selector in the GDT into the task register
 */
static void load_task_register(uint16_t selector)
{
    asm volatile ("ltr %0" : : "r"(selector) : );
}

/**
 * Initialize the global descriptor table
 * 
 * There are certain segments which absolutely MUST be in the x86 GDT:
 *  - Null descriptor (gate 0, descriptor 0x0) - must be zeroed
 *  - Kernel CS (gate 1, descriptor 0x08)
 *  - Kernel DS (gate 2, descriptor 0x10)
 *  - User CS (gate 3, descriptor 0x18)
 *  - User DS (gate 4, descriptor 0x20)
 *  - TSS descriptor - must hold the address of the task state segment
 *        for the processor. The stack pointer is reloaded from the
 *        ss0:esp0 fields in the TSS on task switches (int 0x80).
 *        In our implementation this is gate 5, descriptor 0x28 - however,
 *        this particular placement is NOT required.
 *
 * The above segments are mandated by Intel in order to transfer control
 *  from CPL 3 to CPL 0 (and back again). Other than that, we don't use
 *  the GDT at all ;)
 */
static void gdt_init() 
{
    puts("Initializing GDT...\n");

    gdt_ptr.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gdt_ptr.base = (uint32_t)&gdt_entries;
    gdt_set_gate(0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A); // CS
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92); // DS
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA); // userland CS
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2); // userland DS
        
    gdt_set_gate(5, (uint32_t)&tss, sizeof(tss), 0x89);

    gdt_flush((uint32_t)&gdt_ptr);

    tss.ss0 = 0x10;
    tss.esp0 = (uint32_t)ld_tss_stack + 4092;
    tss.iopb_offset = sizeof(tss);
    load_task_register(0x28);
}

/**
 * Initialize the interrupt descriptor table
 *
 * Each interrupt the kernel wishes to handle must be entered in the IDT.
 * The isrN and irqN functions here are assembler stubs (see interrupts.s)
 * which simply save the appropriate thread context and interrupt code and
 * jump to the registered handler (see interrupt.c).
 */
static void idt_init()
{
    puts("Initializing IDT...\n");
    idt_ptr.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_ptr.base = (uint32_t)&idt_entries;
    memset(&idt_entries, 0, sizeof(struct idt_entry) * 256);
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

    idt_set_gate(0x80, (uint32_t)isr128, 0x0B, 0xEE);

    idt_flush((uint32_t)&idt_ptr);
    puts("IDT flushed.\n");
}

static void gdt_set_gate(uint32_t entry, uint32_t base,
                         uint32_t limit, uint8_t access)
{
    uint32_t flags = (1 << 2);
    if (limit > 0xFFFF) {
        limit = limit >> 12;
        flags |= (1 << 3);
    }

    uint32_t limit_low = limit & 0xFFFF;
    uint32_t limit_high = (limit >> 16) & 0xF;

    uint32_t base_low = base & 0xFFFF;
    uint32_t base_middle = (base >> 16) & 0xFF;
    uint32_t base_high = (base >> 24) & 0xFF;

    uint32_t entry_low = limit_low | (base_low << 16);
    uint32_t entry_high = base_middle | (access << 8) | (limit_high << 16)
        | (flags << 20) | (base_high << 24);

    gdt_entries[entry].low = entry_low;
    gdt_entries[entry].high = entry_high;
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
    outb(PIC2_C, 0x11);

    outb(PIC1_D, master); // vector offsets (new ISR numbers)
    outb(PIC2_D, slave);

    outb(PIC1_D, 0x04); // daisy-chaining IRQ locations
    outb(PIC2_D, 0x02);

    outb(PIC1_D, 0x01); // 8088 mode
    outb(PIC2_D, 0x01);

    outb(PIC1_D, mask1);
    outb(PIC2_D, mask2);

    puts("PIC remapped.\n");
}

void flush_idt()
{
    idt_flush((uint32_t)&idt_ptr);
}

void enable_irq(uint8_t irq)
{
    if (irq < 8) {
        uint8_t mask = inb(PIC1_D);
        SET_BIT(mask, irq);
        outb(PIC1_D, mask);
    }
    else {
        uint8_t mask = inb(PIC2_D);
        SET_BIT(mask, irq - 8);
        outb(PIC1_D, mask);
    }
}

void disable_irq(uint8_t irq)
{
    if (irq < 8) {
        uint8_t mask = inb(PIC1_D);
        CLR_BIT(mask, irq);
        outb(PIC1_D, mask);
    }
    else {
        uint8_t mask = inb(PIC2_D);
        CLR_BIT(mask, irq - 8);
        outb(PIC1_D, mask);
    }
}

void set_esp0(uint32_t new)
{
    tss.esp0 = new;
}
