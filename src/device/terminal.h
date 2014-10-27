#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>
	/* terminal dimensions */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

/* terminal location */
size_t terminal_row;
size_t terminal_column;

/* current terminal colors + buffer base */
uint8_t terminal_color;
uint16_t* terminal_buffer;

/**
 * VGA color entry (4 bits)
 * two entries make a foreground and
 * background color from 8-bit unsigned int
 */

typedef enum vga_color 
{
	COLOR_BLACK = 0,
	COLOR_BLUE,
	COLOR_GREEN,
	COLOR_CYAN,
	COLOR_RED,
	COLOR_MAGENTA,
	COLOR_BROWN,
	COLOR_LIGHT_GREY,
	COLOR_DARK_GREY,
	COLOR_LIGHT_BLUE,
	COLOR_LIGHT_GREEN,
	COLOR_LIGHT_CYAN,
	COLOR_LIGHT_RED,
	COLOR_LIGHT_MAGENTA,
	COLOR_LIGHT_BROWN,
	COLOR_WHITE,
} vga_color;

uint8_t make_color(vga_color fg, vga_color bg);
uint16_t make_vgachar(char c, uint8_t col);

void terminal_init();
void terminal_setcolor(uint8_t color);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
void putc(char c);
void puts(const char *str);
void puti(int32_t i);
void putui(uint32_t i);
void puth(uint32_t i);
void putuh(uint32_t i);
