#ifndef __PRINTF_H_
#define __PRINTF_H_

#include <stdarg.h>

void printf(const char *format, ...);
void vprintf(const char *format, va_list args);

#endif // __PRINTF_H_
