#ifndef __ALGORITHM_H_
#define __ALGORITHM_H_

#include <stddef.h>

#define min(a, b) ({__typeof__(a) _a = a; \
    __typeof__(b) _b = b; \
    _b < _a ? _b : _a;})

#define max(a, b) ({__typeof__(a) _a = a; \
    __typeof__(b) _b = b; \
    _b > _a ? _b : _a;})

#endif // __ALGORITHM_H_
