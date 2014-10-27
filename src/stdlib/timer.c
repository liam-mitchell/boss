#include "timer.h"
#include "interrupt.h"
#include "terminal.h"
#include "device_io.h"

uint32_t ticks = 0;

static void timer_callback(registers_t *registers)
{
	++ticks;
	puts("Tick: ");
	putui(ticks);
	putc('\n');
}

void timer_init(uint32_t frequency)
{
	register_interrupt_callback(32, &timer_callback);

	uint32_t divisor = 1193180 / frequency;
	outb(0x43, 0b00110100);

	uint8_t low = (uint8_t)(divisor & 0xFF);
	uint8_t high = (uint8_t)( (divisor >> 8) & 0xFF);

   	outb(0x40, low);
   	outb(0x40, high);
}
