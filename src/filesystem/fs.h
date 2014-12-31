#ifndef __FS_H_
#define __FS_H_

#include <stdint.h>

#include "macros.h"

#define NAME_MAX 128

#define FS_FILE (1 << 0)
#define FS_DIR (1 << 1)
#define FS_CDEV (1 << 2)
#define FS_BDEV (1 << 3)
#define FS_PIPE (1 << 4)
#define FS_SYM (1 << 5)
#define FS_MNT (1 << 6)

#define FSID_INITRD (1 << 0)

#define MODE_READ (1 << 0)
#define MODE_WRITE (1 << 1)

#define MAX_OPEN 1024

typedef struct inode inode_t;

typedef struct superblock_ops superblock_ops_t;
typedef struct inode_ops inode_ops_t;
typedef struct file_ops file_ops_t;

typedef struct superblock {
    struct superblock *next;
    inode_t *mount;
    superblock_ops_t *ops;
    char name[0];
} superblock_t;

typedef struct superblock_ops {
    inode_t *(*alloc_inode)(struct superblock *);
    void (*read_inode)(struct superblock *, inode_t *);
    void (*write_inode)(struct superblock *, inode_t *);
    void (*delete_inode)(struct superblock *, inode_t *);
} superblock_ops_t;

typedef struct inode {
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t ino;
    uint32_t permissions;
    struct inode *link;
    inode_ops_t *ops;
    file_ops_t *fops;
} inode_t;

typedef struct inode_ops {
    int (*create)(inode_t *, inode_t *);
    int (*remove)(inode_t *, inode_t *);
    uint32_t (*lookup)(inode_t *, char *);
} inode_ops_t;

typedef struct file {
    inode_t *inode;
    uint32_t length;
    uint32_t offset;
    uint8_t mode;
    file_ops_t *ops;
} file_t;

typedef struct file_ops {
    uint32_t (*read)(file_t *, uint32_t *, uint32_t, void *);
    uint32_t (*write)(file_t *, uint32_t *, uint32_t, void *);
    int (*open)(file_t *, inode_t *, uint8_t);
    void (*close)(file_t *);
} file_ops_t;

superblock_t *mounts;

void init_filesystem();
void mount(superblock_t *sb);

file_t *open_path(const char *pathname, const uint8_t mode);

#endif // __FS_H_
