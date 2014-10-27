#ifndef __DEVICE_IO_H
#define __DEVICE_IO_H

#include <stdint.h>

void outb(uint16_t port, uint8_t b);
void outw(uint16_t port, uint16_t w);
void outl(uint16_t port, uint32_t l);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void io_wait();

#endif // __DEVICE_IO_H
