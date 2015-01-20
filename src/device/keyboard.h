#ifndef __KEYBOARD_H_
#define __KEYBOARD_H_

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

typedef unsigned char keymap[128];

struct tty;

void init_keyboard(void);
int set_keyboard_tty(struct tty *tty);

#endif // __KEYBOARD_H_
