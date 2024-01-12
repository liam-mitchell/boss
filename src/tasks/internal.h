#ifndef __TASK_INTERNAL_H_
#define __TASK_INTERNAL_H_

struct task *alloc_task();
void free_task(struct task *task);

uint32_t alloc_kstack();
void free_kstack(uint32_t esp0);

void task_queue_add(struct task **queue, struct task *task);
void task_queue_remove(struct task **queue, struct task *task);
struct task *task_queue_find(struct task **queue, uint32_t pid);

unsigned long setup_stack(struct task *task, void *data, unsigned long size);

extern struct task *running;
extern struct task *blocked;
extern struct task *idle;

#define switch_context(new) do {                                        \
    set_esp0(new->esp0);                                                \
    uint32_t eax;                                                       \
    asm volatile ("movl %1, %%esp\n\t"                                  \
                  "pop %%eax\n\t"                                       \
                  "mov %%ax, %%ds\n\t"                                  \
                  "mov %%ax, %%es\n\t"                                  \
                  "mov %%ax, %%fs\n\t"                                  \
                  "mov %%ax, %%gs\n\t"                                  \
                  "popa\n\t"                                            \
                  "add $8, %%esp\n\t"                                   \
                  "iret\n\t"                                            \
                  : "=a"(eax)                                           \
                  : "g"(new->esp0)                                      \
                  : "%ebx", "%ecx", "%edx", "%esi", "%edi");            \
    } while (0);


#endif // __TASK_INTERNAL_H_
