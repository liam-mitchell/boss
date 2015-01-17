#include "syscalls.h"

#include "compiler.h"
#include "errno.h"
#include "interrupt.h"
#include "printf.h"
#include "task.h"
#include "vmm.h"

static int sys_write(char __user *buf, uint32_t len);
static int sys_exec(const char __user *path);
static int sys_fork();

static void *syscalls[] = {
    0, /* sys_setup */
    0, /* sys_exit */
    sys_fork,
    0, /* sys_read */
    sys_write,
    0, /* sys_open */
    0, /* sys_close */
    0, /* sys_waitpid */
    0, /* sys_creat */
    0, /* sys_link */
    0, /* sys_unlink */
    sys_exec, /* sys_execve */
};

static uint32_t nsyscalls = ARRAY_SIZE(syscalls);

static int sys_exec(const char __user *path)
{
    if (!check_user_ptr(path)) {
        return -EFAULT;
    }

    exec(path);

    return -1; /* exec shouldn't come back! */
}

static int sys_fork()
{
    return fork();
}

static int sys_write(char __user *buf, uint32_t len)
{
    if (!check_user_ptr(buf) || !check_user_ptr(buf + len)) {
        return -EFAULT;
    }

    for (uint32_t i = 0; i < len; ++i) {
        printf("%c", buf[i]);
    }

    return len;
}

static void syscall_handler(registers_t *regs)
{
    run_queue->regs = *regs;

    if (regs->eax >= nsyscalls || !syscalls[regs->eax]) {
        regs->eax = -ENOSYS;
        return;
    }

    /* printf("found syscall %x\n", regs->eax); */
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

    /* printf("syscall returned %x\n", regs->eax); */
    /* print_regs(regs, "after syscall, return to registers"); */
    /* printf("next word at eip %x: %x\n", regs->eip, *(uint32_t *)regs->eip); */
}

void init_syscalls()
{
    printf("Initializing system calls...\n");
    register_interrupt_callback(0x80, &syscall_handler);
}
