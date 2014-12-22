#ifndef __STRING_H_
#define __STRING_H_

#include <stddef.h>

size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t sz);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t len);

int find(const char c, const char *str);

#endif
