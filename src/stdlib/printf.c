#include "printf.h"

#include <stdarg.h>

#include "terminal.h"

void vprintf(const char *format, va_list args)
{
    char c;
    int i;
    unsigned int u;
    char *s;

    while ((c = *format++)) {
        if (c != '%') {
            putc(c);
        }
        else {
            c = *format++;
            switch(c) {
            case 'c':
                i = va_arg(args, int);
                putc((char)i);
                break;
            case 'd':
                i = va_arg(args, int);
                puti(i);
                break;
            case 'x':
                i = va_arg(args, int);
                puth(i);
                break;
            case 's':
                s = (char *)va_arg(args, char *);
                puts(s);
                break;
            case 'u':
                u = va_arg(args, unsigned int);
                putui(u);
                break;
            case '%':
                putc(c);
                break;
            }
        }
    }
}

void printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vprintf(format, args);

    va_end(args);
}
