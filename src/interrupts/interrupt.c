#include "interrupt.h"

#include "descriptor_tables.h"
#include "device_io.h"
#include "printf.h"

#ifdef __cplusplus
extern "C" {
#endif

interrupt_callback callbacks[256];

void interrupt_handler(registers_t *registers)
{
    /* printf("Recieved interrupt %x (%d)\n", registers->interrupt, registers->interrupt); */
    
    if (callbacks[registers->interrupt] != NULL) {
        interrupt_callback cb = callbacks[registers->interrupt];
        /* printf("Calling callback @%x\n", cb); */
        cb(registers);
    }
}

void irq_handler(registers_t *registers)
{
    outb(0x20, 0x20);
    if (registers->interrupt >= 0x28) outb(0xA0, 0x20);

    if (registers->interrupt > 0x2D) printf("Recieved IRQ %x\n", registers->interrupt);

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
