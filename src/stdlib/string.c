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
