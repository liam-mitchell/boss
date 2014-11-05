#ifndef __MACROS_H_
#define __MACROS_H_

#define EXTERN extern "C" {
#define ENDEXTERN }

#include "terminal.h"

#define PANIC(msg) \
    puts("[PANIC] " msg); \
    while(1);

#endif /* __MACROS_H_ */
