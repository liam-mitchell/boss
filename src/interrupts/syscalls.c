#include "syscalls.h"

#include "compiler.h"
#include "errno.h"
#include "printf.h"
#include "task.h"

#include "device/interrupt.h"
#include "fs/vfs.h"
#include "memory/vmm.h"

static int sys_fork(void);
static void sys_exit(int code);
static int sys_read(int fd, char __user *buf, uint32_t len);
static int sys_write(int fd, char __user *buf, uint32_t len);
static int sys_wait(uint32_t pid, int __user *status);
static int sys_open(char __user *path, uint8_t mode);
static int sys_close(int fd);
static int sys_exec(const char __user *path);
static int sys_yield(void);

extern void restore_context(registers_t *new);

static void *syscalls[] = {
    0,                          /* 0 */
    sys_exit,
    sys_fork,
    sys_read,
    sys_write,
    sys_open,                   /* 5 */
    sys_close,
    sys_wait,
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

static void sys_exit(int code)
{
    exit(code);
}

static int sys_wait(uint32_t pid, int __user *status)
{
    return wait(pid, status);
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
    /* current_task->regs = *regs; */
    /* printf("handling syscall %x for current task %u\n", regs->eax, current_task->pid); */

    if (regs->eax >= nsyscalls || !syscalls[regs->eax]) {
        regs->eax = -ENOSYS;
        return;
    }

    /* printf("syscall addr %x\n", syscalls[regs->eax]); */
    /* print_regs(regs, "before syscall, come from registers"); */
    
    void *syscall = syscalls[regs->eax];
    asm volatile ("push %0\n\t"
                  "push %1\n\t"
                  "push %2\n\t"
                  "push %3\n\t"
                  "push %4\n\t"
                  "push %5\n\t"
                  "call *%6\n\t"
                  "add $24, %%esp\n\t"
                  : 
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
    register_interrupt_handler(0x80, &syscall_handler);
}
