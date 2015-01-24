#ifndef __MEMORY_H_
#define __MEMORY_H_

#include <stddef.h>

/**
 * implements standard library memset
 */
void *memset(void *ptr, int value, size_t num);

void *memcpy(void *dest, const void *src, size_t sz);
#endif // __MEMORY_H_
