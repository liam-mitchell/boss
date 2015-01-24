#include "device/terminal.h"

#include <stddef.h>

#include "ldsymbol.h"
#include "macros.h"
#include "string.h"

extern ldsymbol ld_screen;

/* terminal location */
static size_t terminal_row = 0;
static size_t terminal_column = 0;

/* current terminal colors + buffer base */
static uint8_t terminal_color = COLOR_LIGHT_GREY | (COLOR_BLACK << 4);
static uint16_t *terminal_buffer = (uint16_t *)0xB8000;

/**
 * scroll terminal up one line
 */
static void terminal_scroll()
{
    for (size_t y = 0; y < VGA_HEIGHT - 1; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            terminal_buffer[y * VGA_WIDTH + x] =
                terminal_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; ++x) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            make_vgachar(' ', terminal_color);
    }

    --terminal_row;
}

/**
 * creates a VGA color from foreground + backgroud
 */
uint8_t make_color(vga_color fg, vga_color bg)
{
    return fg | bg << 4;
}

/**
 * makes a VGA character entry of the specified color
 * upper 8 bits are color,
 * lower 8 bits are ASCII code
 */
uint16_t make_vgachar(char c, uint8_t col)
{
    uint16_t vgachar = c;
    uint16_t vgacolor = col;
    return vgachar | vgacolor << 8;
}

/**
 * initializes the terminal's buffer to point to
 * memory-mapped location of the VGA buffer for the screen
 * as well as characters to blank black
 */
void init_terminal()
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    terminal_buffer = (uint16_t *)ld_screen;

    for (size_t y = 0; y < VGA_HEIGHT; ++y) {
        for (size_t x = 0; x < VGA_WIDTH; ++x) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = make_vgachar(' ', terminal_color);
        }
    }
}

/**
 * sets the terminal output color
 */
void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
}

/**
 * puts a colored character at a specific x/y in the terminal
 */
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = make_vgachar(c, color);
}

/**
 * writes a character to the buffer
 * at the current location with the current color
 */
void putc(char c)
{
    if (c == '\n') {
        ++terminal_row;

        if (terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }

        terminal_column = 0;
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);

    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        ++terminal_row;

        if (terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
}

/**
 * writes a null-terminated string to the buffer
 * at the current cursor location with the current color
 */
void puts(const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i) putc(str[i]);
}


/**
 * writes a signed 32-bit int to the terminal
 */
void puti(int32_t i)
{
    uint32_t u;

    if (i > 0) {
        u = i;
    }
    else {
        u = -i;
        putc('-');
    }

    putui(u);
}

/**
 * writes an unsigned 32-bit int to the terminal
 */
void putui(uint32_t i)
{
    char c;	
    char string[11];
    int pos = 9; // read digits from least significant -> most significant

    while (i) {
        c = (i % 10) + '0';
        string[pos] = c;
        i = i / 10;
        --pos;
    }

    string[10] = '\0';

    ++pos;

    if (pos == 10)
        putc('0');

    puts(string + pos);
}

/**
 * writes a hexadecimal 32-bit int to the terminal
 */
void puth(uint32_t i)
{
    char c;
    char string[9];
    int j = 7;

    puts("0x");

    while (j >= 0) {
        c = i % 16;

        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;

        string[j] = c;
        i = i / 16;
        --j;
    }

    string[8] = '\0';
    puts(string);
}

void putuh(uint32_t i)
{
    char c;
    char string[9];
    int j = 7;

    while (j >= 0) {
        c = i % 16;

        if (c < 10)
            c += '0';
        else
            c += 'A' - 10;

        string[j] = c;
        i = i / 16;
        --j;
    }

    string[8] = '\0';
    puts(string);
}
