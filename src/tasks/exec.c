#include "task.h"
#include "internal.h"

#include "errno.h"
#include "ldsymbol.h"

#include "device/descriptor_tables.h"
#include "fs/vfs.h"
#include "memory/kheap.h"

extern ldsymbol ld_virtual_offset;

int exec(const char *path)
{
    int err = 0;
    file_t *binary = open_path(path, MODE_READ);
    if (!binary) {
        err = -ENOENT;
        goto error;
    }

    void *data = kmalloc(MEM_GEN, binary->length);
    if (!data) {
        err = -ENOMEM;
        goto error_file;
    }

    uint32_t len = vfs_read(binary, &binary->offset, binary->length, data);
    if (len < binary->length) {
        err = -EIO;
        goto error_filedata;
    }

    free_address_space(current_task->as);
    current_task->as = alloc_address_space();
    if (!current_task->as) {
        err = -ENOMEM;
        goto error_filedata;
    }

    // Set up the stack as if an interrupt had just occured *right* where
    //   we want the task to begin, so switch_context() can iret smoothly
    // See Intel developer's manual 6.4.1 (page 6-9)
    uint32_t stack[] = {
        0x23, // DS, ES, FS, GS
        0, 0, 0, 0, 0, 0, 0, 0, // general-purpose registers
        0, 0, // interrupt number, error code
        0, // eip
        0x1B, // CS
        (1 << 9), // eflags      
        (uint32_t)ld_virtual_offset - 4, // eip
        0x23, // ss
    };

    err = setup_stack(current_task, stack, sizeof(stack));
    if (err < 0) {
        goto error_as;
    }

    err = map_as_data(current_task->as, len, data);
    if (err < 0) {
        goto error_kstack;
    }

    err = map_as_stack(current_task->as);
    if (err < 0) {
        goto error_kstack;
    }

    switch_address_space(NULL, current_task->as);
    switch_context(current_task);

 error_kstack:
    free_kstack(current_task->esp0);

 error_as:
    free_address_space(current_task->as);

 error_filedata:
    kfree(data);

 error_file:
    vfs_close(binary);

 error:
    return err;
}
