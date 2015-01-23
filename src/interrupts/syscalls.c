#include "syscalls.h"

#include "compiler.h"
#include "errno.h"
#include "interrupt.h"
#include "printf.h"
#include "task.h"
#include "vfs.h"
#include "vmm.h"

static int sys_fork(void);
static int sys_read(int fd, char __user *buf, uint32_t len);
static int sys_write(int fd, char __user *buf, uint32_t len);
static int sys_open(char __user *path, uint8_t mode);
static int sys_close(int fd);
static int sys_exec(const char __user *path);
static int sys_yield(void);

extern void restore_context(registers_t *new);

static void *syscalls[] = {
    0, /* sys_setup */          /* 0 */
    0, /* sys_exit */
    sys_fork,
    sys_read,
    sys_write,
    sys_open,                   /* 5 */
    sys_close,
    0, /* sys_waitpid */
    0, /* sys_creat */
    0, /* sys_link */
    0, /* sys_unlink */         /* 10 */
    sys_exec, /* sys_execve */
    sys_yield,
};

static uint32_t nsyscalls = ARRAY_SIZE(syscalls);

static int sys_exec(const char __user *path)
{
    if (!check_user_ptr(path)) {
        return -EFAULT;
    }

    return exec(path);
}

static int sys_fork(void)
{
    return fork();
}

static int sys_read(int fd, char __user *buf, uint32_t len)
{
    /* printf("sys_read: handling read syscall from pid %d at %x\n", current_task->pid, current_task->regs.eip); */
    if (!check_user_ptr(buf) || !check_user_ptr(buf + len)) {
        return -EFAULT;
    }

    file_t *file = current_task->files[fd];

    if (!file) {
        return -ENOENT;
    }

    return vfs_read(file, &file->offset, len, buf);
}

static int sys_write(int fd, char __user *buf, uint32_t len)
{
    /* printf("sys_write: handling write syscall from pid %d: %s (len %d)\n", current_task->pid, buf, len); */
    if (!check_user_ptr(buf) || !check_user_ptr(buf + len)) {
        return -EFAULT;
    }

    /* for (uint32_t i = 0; i < len; ++i) { */
    /*     printf("%c", buf[i]); */
    /* } */

    file_t *file = current_task->files[fd];

    if (!file) {
        return -ENOENT;
    }

    return vfs_write(file, &file->offset, len, buf);
}

static int sys_open(char __user *path, uint8_t mode)
{
    if (!check_user_ptr(path)) {
        return -EFAULT;
    }

    file_t *file = open_path(path, mode);
    if (!file) {
        return -ENOENT;
    }

    for (int i = 0; i < TASK_MAX_FILES; ++i) {
        if (!current_task->files[i]) {
            current_task->files[i] = file;
            return i;
        }
    }

    vfs_close(file);
    return -ENFILE;
}

static int sys_close(int fd)
{
    file_t *file = current_task->files[fd];
    if (file) {
        vfs_close(file);
        return 0;
    }

    return -ENOENT;
}

static int sys_yield(void)
{
    /* printf("task %d yielding (esp0 %x)...\n", current_task->pid, current_task->esp0); */
    /* registers_t *regs = (registers_t *)current_task->esp0; */
    /* print_regs(regs, "task regs"); */
    switch_tasks();
    return 0;
}

static void syscall_handler(registers_t *regs)
{
    current_task->regs = *regs;

    if (regs->eax >= nsyscalls || !syscalls[regs->eax]) {
        regs->eax = -ENOSYS;
        return;
    }

    /* printf("handling syscall %x\n", regs->eax); */
    /* printf("syscall addr %x\n", syscalls[regs->eax]); */
    /* print_regs(regs, "before syscall, come from registers"); */
    
    void *syscall = syscalls[regs->eax];
    asm volatile ("push %1\n\t"
                  "push %2\n\t"
                  "push %3\n\t"
                  "push %4\n\t"
                  "push %5\n\t"
                  "push %6\n\t"
                  "call *%7\n\t"
                  "add $24, %%esp\n\t"
                  : "=g"(regs->eax)
                  : "g"(regs->ebp), "g"(regs->edi),
                    "g"(regs->esi), "g"(regs->edx),
                    "g"(regs->ecx), "g"(regs->ebx),
                    "r"(syscall)
                  :);

    /* printf("  syscall returned %x\n", regs->eax); */
    /* print_regs(regs, "after syscall, return to registers"); */
    /* printf("next word at eip %x: %x\n", regs->eip, *(uint32_t *)regs->eip); */

    /* restore_context(&current_task->regs); */
}

void init_syscalls(void)
{
    printf("Initializing system calls...\n");
    register_interrupt_callback(0x80, &syscall_handler);
}
