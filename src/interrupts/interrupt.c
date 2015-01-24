#include "device/interrupt.h"

#include "printf.h"
#include "task.h"

#include "device/descriptor_tables.h"
#include "device/port.h"

interrupt_handler callbacks[256];

static void send_pic_eoi(uint32_t interrupt)
{
    outb(0x20, 0x20);
    if (interrupt >= 0x28) outb(0xA0, 0x20);
}

void handle_interrupt(registers_t *registers)
{
    if (current_task) {
        current_task->esp0 = (uint32_t)registers;
    }

    if (callbacks[registers->interrupt] != NULL) {
        interrupt_handler cb = callbacks[registers->interrupt];
        cb(registers);
    }
}

void handle_irq(registers_t *registers)
{
    send_pic_eoi(registers->interrupt);
    handle_interrupt(registers);
}

void register_interrupt_handler(uint8_t n, interrupt_handler cb)
{
    printf("Registering callback %x (%d)\n", n, n);
    callbacks[n] = cb;
}
