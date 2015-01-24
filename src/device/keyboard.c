#include "device/keyboard.h"

#include <stddef.h>

#include "bits.h"
#include "errno.h"
#include "printf.h"
#include "ring-buffer.h"
#include "task.h"

#include "device/interrupt.h"
#include "device/port.h"
#include "device/tty.h"

#define KBD_DATA 0x60
#define KBD_CTL 0x64

static uint8_t mods = 0;
static struct tty *tty;

static keymap keymap_us = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    KEY_CTL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', KEY_SHIFT, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_SHIFT, '*', KEY_ALT, ' ',
    KEY_CAPSLOCK, /* capslock */
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
    KEY_NUMLOCK, KEY_SCROLLLOCK, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0,
    KEY_F11, KEY_F12, 0
};

static void keyboard_handler(registers_t __unused *regs)
{
    /* printf("handling keyboard event\n"); */
    unsigned char scancode = inb(KBD_DATA);

    unsigned char keycode = KEYCODE(scancode);
    unsigned char key = keymap_us[keycode];

    if (tty && !KEY_IS_UP(scancode)) {
        /* printf("device/keyboard.handler: read key %c from keyboard\n", key); */
        ring_buffer_write(tty->kbd_in, &mods, sizeof(mods));
        ring_buffer_write(tty->kbd_in, &key, sizeof(scancode));
        wake(tty->fg_pid);
        /* printf("device/keyboard.handler: wrote ring buffer 2 bytes\n"); */
    }

    if (KEY_IS_UP(scancode)) {
        switch (key) {
        case KEY_SHIFT:
            CLR_BITS(mods, MOD_SHIFT);
            break;
        case KEY_CTL:
            CLR_BITS(mods, MOD_CTL);
            break;
        case KEY_ALT:
            CLR_BITS(mods, MOD_ALT);
            break;
        default:
            break;
        }
    }
    else {
        switch (key) {
        case KEY_SHIFT:
            SET_BITS(mods, MOD_SHIFT);
            break;
        case KEY_CTL:
            SET_BITS(mods, MOD_CTL);
            break;
        case KEY_ALT:
            SET_BITS(mods, MOD_ALT);
            break;
        default:
            break;
        }
    }

    /* printf("device/keyboard.handler: exiting\n"); */
}

void init_keyboard(void)
{
    register_interrupt_handler(0x21, keyboard_handler);
    inb(KBD_DATA);
}

int set_keyboard_tty(struct tty *new_tty)
{
    if (new_tty) {
        tty = new_tty;
        return 0;
    }

    return -EINVAL;
}
