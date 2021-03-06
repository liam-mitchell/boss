#ifndef __PORT_H_
#define __PORT_H_

#include <stdint.h>

typedef uint16_t port_t;

extern void outb(port_t port, uint8_t data);
extern void outw(port_t port, uint16_t data);
extern void outl(port_t port, uint32_t data);
extern uint8_t inb(port_t port);
extern uint16_t inw(port_t port);
extern uint32_t inl(port_t port);

static inline void io_wait()
{
    outb(0x80, 0);
}


#endif /* __PORT_H_ */
