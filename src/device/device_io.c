#include "device_io.h"

void outb(uint16_t port, uint8_t b)
{
	asm volatile ("outb %%al, %%dx" : : "d" (port), "a" (b));
}

void outw(uint16_t port, uint16_t w)
{
	asm volatile ( "outw %0, %1" : : "a"(w), "dN"(port) );
}

void outl(uint16_t port, uint32_t l)
{
	asm volatile ( "outl %0, %1" : : "a"(l), "dN"(port) );
}

uint8_t inb(uint16_t port)
{
	uint8_t b;
	asm volatile ( "inb %%dx, %%al" : "=a" (b) : "d" (port) );
	return b;
}

uint16_t inw(uint16_t port)
{
	uint16_t w;
	asm volatile ( "inw %1, %0" : "=a"(w) : "Nd"(port) );
	return w;
}

uint32_t inl(uint16_t port)
{
	uint32_t l;
	asm volatile ( "inl %1, %0" : "=a"(l) : "Nd"(port) );
	return l;
}

void io_wait()
{
	asm volatile ( "jmp 1f\n\t"
				   "1:jmp 2f\n\t"
				   "2:" );
}
