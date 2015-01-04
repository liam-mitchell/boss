#include "initrd.h"

#include "errno.h"
#include "fs.h"
#include "kheap.h"
#include "macros.h"
#include "memory.h"
#include "printf.h"
#include "string.h"

#include <stdint.h>

#define INITRD_MAX_FILES 64
#define INITRD_MAGIC 0x0BADC0DE

typedef struct initrd_file {
    uint32_t offset;
    uint32_t length;
    uint32_t flags;
} initrd_file_t;

static initrd_file_t initrd_files[INITRD_MAX_FILES];

static uint32_t initrd_start;
static uint32_t initrd_num;

static void initrd_read_inode(superblock_t *sb, inode_t *inode);

static uint32_t initrd_lookup(inode_t *parent, char *name);

static uint32_t initrd_read(file_t *file, uint32_t *offset,
                            uint32_t size, void *buf);
static int initrd_open(file_t *file, inode_t *inode, uint8_t mode);
static void initrd_close(file_t *file);

static superblock_ops_t initrd_sops = {
    .alloc_inode = NULL,
    .read_inode = initrd_read_inode,
    .write_inode = NULL,
    .delete_inode = NULL
};

static inode_ops_t initrd_iops = {
    .create = NULL,
    .remove = NULL,
    .lookup = initrd_lookup
};

static file_ops_t initrd_fops = {
    .read = initrd_read,
    .write = NULL,
    .open = initrd_open,
    .close = initrd_close
};

static void initrd_read_inode(superblock_t *sb, inode_t *inode)
{
    UNUSED(sb);
    if (inode->ino > initrd_num) {
        return;
    }

    inode->uid = 0;
    inode->gid = 0;
    inode->flags = initrd_files[inode->ino].flags;
    inode->permissions = 0x0755;
    inode->link = NULL;
    inode->ops = &initrd_iops;
    inode->fops = &initrd_fops;
}

static uint32_t initrd_lookup(inode_t *dir, char *name)
{
    puts("looking up ");
    puts(name);
    puts(" in dir with ino ");
    puti(dir->ino);
    putc('\n');

    if (dir->flags != FS_DIR) {
        puts("attempted lookup in inode that wasn't directory\n");
        return 0;
    }

    initrd_file_t *dir_file = &initrd_files[dir->ino];
    void *dir_data = (void *)initrd_start + dir_file->offset;
    void *dir_end = dir_data + dir_file->length;
    uint32_t ret = 0;

    while (dir_data < dir_end) {
        uint32_t ino;
        char *dir_name;

        memcpy(&ino, dir_data, sizeof(ino));
        dir_name = dir_data + sizeof(ino);

        printf("checking filename %s (length %d)\n"
               " against direntry with name %s\n",
               name, strlen(name), dir_name);

        if (strncmp(dir_name, name, strlen(name)) == 0) {
            memcpy(&ret, dir_data, sizeof(uint32_t));
            break;
        }

        dir_data += sizeof(ino) + strlen(name);
        dir_data = (void *)ALIGN((uint32_t)dir_data, 4);
    }

    return ret;
}

static uint32_t initrd_read(file_t *file, uint32_t *offset,
                            uint32_t size, void *buf)
{
    if (!(file->mode & MODE_READ)) {
        errno = EBADF;
        return 0;
    }

    if (*offset + size > file->length) {
        size = file->length - *offset;
    }

    void *start = (void *)initrd_start
        + initrd_files[file->inode->ino].offset
        + *offset;

    memcpy(buf, start, size);

    return size;
}

static int initrd_open(file_t *file, inode_t *inode, uint8_t mode)
{
    puts("opening initrd file inode #");
    puth(inode->ino);
    putc('\n');

    if (inode->ino > initrd_num) {
        return -ENOENT;
    }

    if (inode->flags & FS_DIR) {
        return -EISDIR;
    }

    puts("filling file data...\n");
    file->inode = inode;
    file->length = initrd_files[inode->ino].length;
    file->offset = 0;
    file->mode = mode;
    file->ops = &initrd_fops;
    puts("filled file data\n");

    return 0;
}

static void initrd_close(file_t *file)
{
    UNUSED(file);
}

superblock_t *init_initrd(uint32_t start)
{
    puts("Initializing initrd @");
    puth(start);
    putc('\n');

    const char *name = "init";
    superblock_t *sb = kzalloc(sizeof(*sb) + strlen(name) + 1);
    strncpy(sb->name, name, strlen(name));
    sb->ops = &initrd_sops;

    initrd_start = start;
    memcpy(&initrd_num, (void *)initrd_start, sizeof(initrd_num));

    puts("number of initrd files: ");
    puth(initrd_num);
    putc('\n');
    
    inode_t *root = kzalloc(sizeof(*root));
    root->uid = 0;
    root->gid = 0;
    root->flags = FS_DIR;
    root->ino = 0;
    root->permissions = 0x0755;
    root->ops = &initrd_iops;

    sb->mount = root;

    memset(initrd_files, 0, sizeof(initrd_file_t) * INITRD_MAX_FILES);

    start += sizeof(initrd_start);
    for (uint32_t i = 0; i < initrd_num; ++i) {
        memcpy(&initrd_files[i], (void *)start, sizeof(initrd_file_t));
        puts("initrd file ");
        puti(i);
        puts(":\n offset ");
        puth(initrd_files[i].offset);
        puts("\n length ");
        puth(initrd_files[i].length);
        puts("\n flags ");
        puth(initrd_files[i].flags);
        putc('\n');

        if (initrd_files[i].flags == FS_FILE) {
            char buf[NAME_MAX];
            strncpy(buf, (char *)initrd_start + initrd_files[i].offset, initrd_files[i].length);
            puts("file data: ");
            puts(buf);
            putc('\n');
        }
        
        start += sizeof(initrd_file_t);
    }

    return sb;
}
