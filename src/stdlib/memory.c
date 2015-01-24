#include "memory/memory.h"

void *memset(void *ptr, int value, size_t num)
{
	char *p = (char *)ptr;
	char *end = p + num;
	while (p != end) *p++ = value;
	return ptr;
}

void *memcpy(void *dest, const void *src, size_t sz)
{
    char *from = (char *)src;
    char *to = (char *)dest;

    for (size_t i = 0; i < sz; ++i) {
        to[i] = from[i];
    }

    return dest;
}
