#include "interrupt.h"
#include "terminal.h"
#include "device_io.h"

#ifdef __cplusplus
extern "C" {
#endif

interrupt_callback callbacks[256];

void interrupt_handler(registers_t *registers)
{
	puts("recieved interrupt: ");
        puth(registers->interrupt);
	putc('\n');
}

void irq_handler(registers_t *registers)
{

	outb(0x20, 0x20);
	if (registers->interrupt >= 0x28) outb(0xA0, 0x20);

	if (callbacks[registers->interrupt] != NULL) {
		interrupt_callback cb = callbacks[registers->interrupt];
		cb(registers);
	}
}

void register_interrupt_callback(uint8_t n, interrupt_callback cb)
{
	puts("registering callback: ");
	puth((uint32_t)n);
	putc('\n');
	callbacks[n] = cb;
	puts("callback registered.\n");
}

#ifdef __cplusplus
}
#endif
