#include "vfs.h"

void vfs_read_inode(superblock_t *sb, inode_t *inode)
{
    if (sb->ops->read_inode) {
        sb->ops->read_inode(sb, inode);
    }
}

uint32_t vfs_lookup(inode_t *inode, char *name)
{
    if (inode->ops->lookup) {
        return inode->ops->lookup(inode, name);
    }

    return 0;
}

uint32_t vfs_read(file_t *file, uint32_t *offset, uint32_t length, void *buf)
{
    if (file->ops->read) {
        return file->ops->read(file, offset, length, buf);
    }

    return 0;
}

uint32_t vfs_write(file_t *file, uint32_t *offset, uint32_t length, void *buf)
{
    if (file->ops->write) {
        return file->ops->write(file, offset, length, buf);
    }

    return 0;
}

int vfs_open(file_t *file, inode_t *inode, uint8_t mode)
{
    if (inode->fops->open) {
        return inode->fops->open(file, inode, mode);
    }

    return 0;
}

void vfs_close(file_t *file)
{
    if (file->ops->close) {
        file->ops->close(file);
    }
}
