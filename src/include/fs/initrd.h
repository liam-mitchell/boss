#ifndef __INITRD_H_
#define __INITRD_H_

#include "fs/fs.h"

superblock_t *init_initrd(uint32_t start);

#endif // __INITRD_H_
