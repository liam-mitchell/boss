#include "keyboard.h"

#include "device_io.h"
#include "interrupt.h"
#include "printf.h"

#include <stddef.h>

#define KBD_DATA 0x60
#define KBD_CTL 0x64

#define KEYCODE(k) ((k) & 0x7F)
#define KEY_UP (1 << 7)
#define KEY_IS_UP(k) ((k) & KEY_UP)

#define MOD_ALT (1 << 0)
#define MOD_SHIFT (1 << 1)
#define MOD_CTL (1 << 2)

#define KEY_ALT 1
#define KEY_SHIFT 2
#define KEY_CTL 3
#define KEY_F1 4
#define KEY_F2 5
#define KEY_F3 6
#define KEY_F4 7
#define KEY_F5 8
#define KEY_F6 9
#define KEY_F7 10
#define KEY_F8 11
#define KEY_F9 12
#define KEY_F10 13
#define KEY_F11 14
#define KEY_F12 15
#define KEY_CAPSLOCK 16
#define KEY_NUMLOCK 17
#define KEY_SCROLLLOCK 18
#define KEY_ESC 19

static uint8_t mods = 0;

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

static void keyboard_handler(registers_t __attribute__((unused))*regs)
{
    unsigned char scancode = inb(KBD_DATA);

    unsigned char keycode = KEYCODE(scancode);
    unsigned char key = keymap_us[keycode];

    if (KEY_IS_UP(scancode)) {
        if (key == KEY_SHIFT) {
            mods &= ~MOD_SHIFT;
        }
        else if (key == KEY_CTL) {
            mods &= ~MOD_CTL;
        }
        else if (key == KEY_ALT) {
            mods &= ~MOD_ALT;
        }
    }
    else {
        printf("key pressed: %c", key);
    }
}

void init_keyboard(void)
{
    register_interrupt_callback(0x21, keyboard_handler);
}
