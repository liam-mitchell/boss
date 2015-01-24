#ifndef __MACROS_H_
#define __MACROS_H_

#define EXTERN extern "C" {
#define ENDEXTERN }

#include "device/terminal.h"

#ifndef NULL
#define NULL (void *)0
#endif

#define UNUSED(x) ((x) = (x))

#define PANIC(msg) \
    puts("[PANIC] " msg); \
    while(1);

#define ALIGN(x, a) __ALIGN_MASK((x),(typeof(x))(a) - 1)
#define __ALIGN_MASK(x, m) ((x + m) & ~(m))

#endif /* __MACROS_H_ */
