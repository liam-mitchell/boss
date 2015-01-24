#ifndef __DEVFS_H_
#define __DEVFS_H_

#include "fs.h"

superblock_t *init_devfs();

int create_device_file(struct file_ops *fops, char *name, uint32_t permissions);

#endif // __DEVFS_H_
