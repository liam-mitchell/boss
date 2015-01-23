#include "tty.h"

#include "bits.h"
#include "devfs.h"
#include "fs.h"
#include "keyboard.h"
#include "kheap.h"
#include "memory.h"
#include "printf.h"
#include "ring-buffer.h"
#include "task.h"

#define TTY_KEYBUF_SIZE 1024

static struct tty tty;

static uint32_t tty_read(file_t *file, uint32_t *offset,
                         uint32_t size, void *buf);
static uint32_t tty_write(file_t *file, uint32_t *offset,
                          uint32_t size, void *buf);
static int tty_open(file_t *file, inode_t *inode, uint8_t mode);
static void tty_close(file_t *file);

static volatile uint32_t dummy;

static struct file_ops tty_fops = {
    .read = tty_read,
    .write = tty_write,
    .open = tty_open,
    .close = tty_close,
};

static uint32_t tty_read(file_t __unused *file, uint32_t __unused *offset,
                         uint32_t size, void *buf)
{
    uint32_t ret = size;

    // Wait until we're in the foreground and the tty has enough data
    while (current_task->pid != tty.fg_pid || tty.kbd_in->size < size * 2) {
        sleep();
    }

    uint8_t *keys = kmalloc(MEM_GEN, size * 2);
    ring_buffer_read(tty.kbd_in, keys, size * 2);

    // Parse mod-key pairs from the keyboard driver
    for (uint32_t i = 0; i < size; ++i) {
        uint8_t mods = keys[i * 2];

        if (TST_BITS(mods, MOD_SHIFT)) {
            keys[i * 2 + 1] += 128; // TODO: capitalize here...
        }

        memcpy(buf, &keys[i * 2 + 1], 1);
    }

    kfree(keys);
    return ret;
}

static uint32_t tty_write(file_t __unused *file, uint32_t __unused *offset,
                          uint32_t size, void *buf)
{
    uint32_t ret = size;
    while (size) {
        printf("%c", *(char *)buf);
        ++buf;
        --size;
    }

    return ret;
}

static int tty_open(file_t *file, inode_t *inode, uint8_t mode)
{
    file->inode = inode;
    file->length = 0;
    file->offset = 0;
    file->mode = mode;
    file->ops = &tty_fops;

    return 0;
}

static void tty_close(file_t __unused *file)
{
    // Empty - the TTY doesn't care if it gets closed (though the process will)
}

void init_tty(void)
{
    if (create_device_file(&tty_fops, "tty", 0x666) < 0) {
        PANIC("Unable to create tty device file!");
    }

    printf("Created TTY device file\n");

    tty.kbd_in = ring_buffer_create(TTY_KEYBUF_SIZE * 2);
    tty.fg_pid = 1;

    if (set_keyboard_tty(&tty) < 0) {
        PANIC("Unable to set keyboard active tty!");
    }

    printf("Initialized tty\n");
}
