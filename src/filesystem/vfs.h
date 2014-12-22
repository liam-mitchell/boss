#ifndef __VFS_H_
#define __VFS_H_

#include <stdint.h>

#include "fs.h"

void vfs_alloc_inode(superblock_t *sb);
void vfs_read_inode(superblock_t *sb, inode_t *inode);
void vfs_write_inode(superblock_t *sb, inode_t *inode);
void vfs_delete_inode(superblock_t *sb, inode_t *inode);

int vfs_create(inode_t *inode, inode_t *parent);
int vfs_remove(inode_t *inode, inode_t *parent);
uint32_t vfs_lookup(inode_t *inode, char *name);

uint32_t vfs_read(file_t *file, uint32_t *offset, uint32_t length, void *buf);
uint32_t vfs_write(file_t *file, uint32_t *offset, uint32_t length, void *buf);
int vfs_open(file_t *file, inode_t *inode, uint8_t mode);
void vfs_close(file_t *file);

#endif
