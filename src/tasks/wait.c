#include "task.h"
#include "internal.h"

#include "errno.h"
#include "printf.h"
#include "memory/vmm.h"

int wait(uint32_t pid, int __user *status)
{
    printf("waiting for process %u from process %u\n", pid, current_task->pid);
    if (!check_user_ptr(status)) {
	return -EFAULT;
    }

    struct list *elem;
    LIST_FOR_EACH(&current_task->children, elem) {
	struct task *child = LIST_ENTRY(elem, struct task, children_list);

	if (child->pid == pid) {
	    while (child->status != TASK_FINISHED) {
		printf("sleeping while waiting for process\n");
		sleep();
	    }
	}

	printf("child process exited, code %d\n", child->exit_code);
	*status = child->exit_code;

	free_task(child);

	return pid;
    }

    return -1;
}
