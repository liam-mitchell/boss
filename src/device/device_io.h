#ifndef __DEVICE_IO_H
#define __DEVICE_IO_H

#include <stdint.h>

extern void outb(uint16_t port, uint8_t b);
extern void outw(uint16_t port, uint16_t w);
extern void outl(uint16_t port, uint32_t l);
extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

void io_wait();

#endif // __DEVICE_IO_H
