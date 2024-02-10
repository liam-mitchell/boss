#ifndef __TEST_H_
#define __TEST_H_

#include "macros.h"

#define KASSERT(x) if (!(x)) {PANIC("kassert failure at " __FILE__ ":" STR(__LINE__) ": " #x)}

void ktest(void);

#endif
