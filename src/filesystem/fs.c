#include "fs.h"

#include "devfs.h"
#include "initrd.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
#include "printf.h"
#include "string.h"
#include "vfs.h"

#define INODE_MAX 1024

extern ldsymbol ld_initrd;

void mount(superblock_t *sb)
{
    sb->next = NULL;

    if (mounts == NULL) {
        puts("mounted fs: ");
        puts(sb->name);
        putc('\n');
        mounts = sb;
        return;
    }

    superblock_t *curr = mounts;
    while (curr->next != NULL) {
        curr = curr->next;
    }

    curr->next = sb;

    puts("mounted fs: ");
    puts(sb->name);
    putc('\n');
}

void init_filesystem()
{
    superblock_t *init = init_initrd((uint32_t)ld_initrd);
    mount(init);

    superblock_t *dev = init_devfs();
    mount(dev);
}

static const char *next_component(char *dest, const char *pathname)
{
    if (*pathname == '/') {
        ++pathname;
    }

    int len = find('/', pathname);
    if (len < 0) {
        len = strlen(pathname);
    }

    strncpy(dest, pathname, len);

    return pathname + len;
}

file_t *open_path(const char *pathname, const uint8_t mode)
{
    const char *pathname_end = pathname + strlen(pathname);
    if (*pathname++ != '/') {
        return NULL;
    }

    char component[NAME_MAX] = { 0 };
    pathname = next_component(component, pathname);

    superblock_t *mount;
    for (mount = mounts; mount != NULL; mount = mount->next) {
        if (strcmp(component, mount->name) == 0) {
            break;
        }
    }

    if (!mount) {
        return NULL;
    }

    inode_t inode = *mount->mount;

    while (pathname < pathname_end) {
        pathname = next_component(component, pathname);
        uint32_t ino = vfs_lookup(&inode, component);

        if (ino == 0) {
            return NULL;
        }

        inode.ino = ino;
        vfs_read_inode(mount, &inode);
    }

    file_t *file = kzalloc(MEM_GEN, sizeof(*file));
    vfs_open(file, &inode, mode);
    
    return file;
}
