#include "fs/devfs.h"

#include "compiler.h"
#include "errno.h"
#include "printf.h"
#include "string.h"

#include "device/tty.h"
#include "fs/fs.h"
#include "memory/kheap.h"

#define DEV_NFILES 4

static inode_t *devfs_alloc_inode(struct superblock *sb);
static void devfs_read_inode(struct superblock *sb, struct inode *inode);

static int devfs_create(struct inode *inode, char *name, uint32_t permissions);
static uint32_t devfs_lookup(struct inode *parent, char *name);

static superblock_ops_t devfs_sops = {
    .alloc_inode = devfs_alloc_inode,
    .read_inode = devfs_read_inode,
    .write_inode = NULL,
    .delete_inode = NULL
};

static inode_ops_t devfs_iops = {
    .create = devfs_create,
    .remove = NULL,
    .lookup = devfs_lookup
};

struct devfs_dentry {
    uint32_t namelen;
    struct devfs_dentry *next;
    char name[0];
};

struct devfs_file {
    struct inode *inode;
    struct file *file;
    char *name;
};

static struct devfs_file devfs_files[DEV_NFILES];
static uint32_t devfs_nfiles = 0;

static int devfs_create(struct inode *inode, char *name, uint32_t permissions)
{
    if (inode->flags != FS_DIR) {
        return -ENODIR;
    }

    struct inode *new = devfs_alloc_inode(NULL);

    if (!new) {
        return -ENFILE;
    }

    new->permissions = permissions;
    devfs_files[new->ino].name = kzalloc(MEM_GEN, strlen(name) + 1);
    strncpy(devfs_files[new->ino].name, name, strlen(name));
    devfs_files[new->ino].inode = new;
    devfs_files[new->ino].file = kzalloc(MEM_GEN, sizeof(struct file));

    return 0;
}

static void devfs_read_inode(struct superblock __unused *sb,
                             struct inode *inode)
{
    if (inode->ino >= devfs_nfiles) {
        return;
    }

    *inode = *devfs_files[inode->ino].inode;
}

static uint32_t devfs_lookup(struct inode *parent, char *name)
{
    if (!(parent->flags == FS_DIR)) {
        errno = ENODIR;
        return 0;
    }

    for (uint32_t i = 1; i < devfs_nfiles; ++i) {
        if (strcmp(devfs_files[i].name, name) == 0) {
            return i;
        }
    }

    errno = -ENOENT;
    return 0;
}

static inode_t *devfs_alloc_inode(struct superblock __unused *sb)
{
    if (devfs_nfiles > DEV_NFILES) {
        return NULL;
    }

    struct inode *inode = kzalloc(MEM_GEN, sizeof(*inode));
    inode->ino = devfs_nfiles;
    inode->ops = &devfs_iops;

    devfs_files[devfs_nfiles].inode = inode;
    devfs_files[devfs_nfiles].file = kzalloc(MEM_GEN, sizeof(struct file));
    ++devfs_nfiles;

    return inode;
}

superblock_t *init_devfs(void)
{
    char *name = "dev";
    superblock_t *mount = kzalloc(MEM_GEN, sizeof(*mount) + strlen(name) + 1);
    strncpy(mount->name, name, strlen(name));
    mount->ops = &devfs_sops;

    inode_t *root = devfs_alloc_inode(NULL);
    root->flags = FS_DIR;
    mount->mount = root;

    devfs_files[0].name = kzalloc(MEM_GEN, strlen(name) + 1);
    strncpy(devfs_files[0].name, name, strlen(name));

    init_tty();
    return mount;
}

/**
 * Create a device file with custom fops to allow devices to register
 * themselves in dev/
 */
int create_device_file(struct file_ops *fops, char *name, uint32_t permissions)
{
    int err = devfs_create(devfs_files[0].inode, name, permissions);
    if (err < 0) {
        return err;
    }

    uint32_t i = devfs_lookup(devfs_files[0].inode, name);
    if (i) {
        devfs_files[i].file->ops = fops;
        devfs_files[i].inode->fops = fops;
        return 0;
    }

    return -EBADF; // TODO: did we really get a bad file descriptor?...
}
