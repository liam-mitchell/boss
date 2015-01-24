#ifndef __COMPILER_H_
#define __COMPILER_H_

#include <stdint.h>

#ifndef NULL
#define NULL (void *)0
#endif

#define __packed __attribute((packed))
#define __unused __attribute((__unused__))
#define __user
#define __section(sct) __attribute((section (sct)))

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define KBYTE (1024)
#define MBYTE (KBYTE * KBYTE)
#define GBYTE (MBYTE * KBYTE)

static inline
uint32_t align(uint32_t x, uint32_t a)
{
    uint32_t mask = a - 1;
    return (x + mask) & ~mask;
}

static inline
void *alignp(void *x, uint32_t a)
{
    return (void *)align((uint32_t)x, a);
}

static inline
uint32_t align_down(uint32_t x, uint32_t a)
{
    uint32_t aligned = align(x, a);
    if (aligned != x) {
        aligned -= a;
    }

    return aligned;
}

#endif // __COMPILER_H_
