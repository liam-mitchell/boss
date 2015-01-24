#include "device/interrupt.h"

#include "device/descriptor_tables.h"
#include "device/port.h"
#include "task.h"
#include "printf.h"

#ifdef __cplusplus
extern "C" {
#endif

interrupt_callback callbacks[256];

void interrupt_handler(registers_t *registers)
{
    if (current_task) {
        /* current_task->regs = *registers; */
        current_task->esp0 = (uint32_t)registers;
    }

    if (callbacks[registers->interrupt] != NULL) {
        interrupt_callback cb = callbacks[registers->interrupt];
        cb(registers);
    }
}

void irq_handler(registers_t *registers)
{
    outb(0x20, 0x20);
    if (registers->interrupt >= 0x28) outb(0xA0, 0x20);

    if (current_task) {
        /* current_task->regs = *registers; */
        current_task->esp0 = (uint32_t)registers;
    }

    /* if (registers->interrupt > 0x2D) printf("Recieved IRQ %x\n", registers->interrupt); */
    /* interrupt_handler(registers); */
    if (callbacks[registers->interrupt] != NULL) {
        interrupt_callback cb = callbacks[registers->interrupt];
        cb(registers);
    }
}

void register_interrupt_callback(uint8_t n, interrupt_callback cb)
{
    printf("Registering callback %x (%d)\n", n, n);
    callbacks[n] = cb;

    /* if (n >= 0x20) { */
        /* enable_irq(n - 0x20); */
    /* } */
}

#ifdef __cplusplus
}
#endif
