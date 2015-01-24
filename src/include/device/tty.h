#ifndef __TTY_H_
#define __TTY_H_

#include "bool.h"
#include "ring-buffer.h"
#include "device/terminal.h"

struct tty {
    struct ring_buffer *kbd_in;
    uint32_t fg_pid;
};

void set_active_tty(uint8_t tty);
void set_foreground_process(uint8_t tty, uint32_t pid);

void init_tty(void);

#endif // __TTY_H_
