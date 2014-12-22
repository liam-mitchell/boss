#include "string.h"

/**
 * standard library strlen()
 * returns the length of a null-terminated C string
 * minus the terminator
 */
size_t strlen(const char *str)
{
	size_t ret = 0;
	while(str[ret]) ++ret;
	return ret;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;

    while(*src) {
        *dest = *src;
        ++dest;
        ++src;
    }

    *dest = '\0';

    return ret;
}

char *strncpy(char *dest, const char *src, size_t sz)
{
    char *ret = dest;

    while (*src && sz > 0) {
        *dest = *src;
        ++dest;
        ++src;
        --sz;
    }

    while (sz > 0) {
        *dest++ = '\0';
    }

    *dest = '\0';

    return ret;
}

int strcmp(const char *a, const char *b)
{
    if (strlen(a) != strlen(b)) {
        return strlen(a) - strlen(b);
    }

    return strncmp(a, b, strlen(a));
}

int strncmp(const char *a, const char *b, size_t len)
{
    while (*a && *b && len) {
        if (*a != *b) {
            break;
        }

        ++a;
        ++b;
        --len;
    }

    return *a - *b;
}

int find(const char c, const char *str)
{
    const char *pos = str;

    while (*pos != c) {
        if (!*pos) {
            return -1;
        }

        ++pos;
    }

    return pos - str;
}
