#include "fs.h"

#include "initrd.h"
#include "kheap.h"
#include "ldsymbol.h"
#include "macros.h"
#include "memory.h"
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
    
    puts("opening path: ");
    puts(pathname);
    putc('\n');

    puts("strlen(pathname): ");
    puti(strlen(pathname));
    putc('\n');

    if (*pathname++ != '/') {
        return NULL;
    }

    char component[NAME_MAX];
    memset(component, 0, NAME_MAX);
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

    puts("opened path on mountpoint: ");
    puts(mount->name);
    puts(" ino ");
    puti(mount->mount->ino);
    putc('\n');

    inode_t *inode = mount->mount;

    while (pathname < pathname_end) {
        pathname = next_component(component, pathname);

        puts("pathname now: ");
        puts(pathname);
        putc('\n');

        puts("looking up component ");
        puts(component);
        puts(" in inode ");
        puti(inode->ino);
        putc('\n');

        uint32_t ino = vfs_lookup(inode, component);

        if (ino == 0) {
            return NULL;
        }

        inode = kzalloc(sizeof(*inode));
        inode->ino = ino;
        vfs_read_inode(mount, inode);

        puts("found inode ");
        puti(ino);
        puts(" flags: ");
        puth(inode->flags);
        putc('\n');
    }

    file_t *file = kzalloc(sizeof(*file));
    vfs_open(file, inode, mode);
    return file;
}
