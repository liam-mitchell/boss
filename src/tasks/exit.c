#include "task.h"
#include "internal.h"

#include "printf.h"

#include "memory/kheap.h"
#include "fs/vfs.h"

static void kill_task_no_wait(struct task *task);

static void kill_children_on_queue(struct task **queue, struct task *task)
{
    if (*queue == NULL) {
	return;
    }
	
    for (struct task *curr = *queue; curr; curr = curr->next) {
	if (curr->parent == task) {
	    kill_task_no_wait(curr);
	}
    }
}

static void kill_children(struct task *task)
{
    kill_children_on_queue(&running, task);
    kill_children_on_queue(&blocked, task);
    kill_children_on_queue(&zombies, task);
}

static void kill_task_no_wait(struct task *task)
{
    kill_children(task);

    for (int i = 0; i < TASK_MAX_FILES; ++i) {
	if (task->files[i]) {
	    printf("closing process file %d\n", i);
	    vfs_close(task->files[i]);
	}
    }

    // TODO: we should maybe track which queue this task is currently on,
    // so as not to require checking all the different queues to figure
    // out where to remove it...
    task_queue_remove_safe(&running, task);
    task_queue_remove_safe(&blocked, task);
    task_queue_remove_safe(&zombies, task);

    kfree(task);
}

static int kill_task(struct task *task)
{
    // TODO: if parent is waiting for us, return code
    // otherwise, zombie us until waited on
    kill_task_no_wait(task);

    return 0;
}

int exit(int code)
{
    printf("exiting process %u with code %d\n", current_task->pid, code);
    return kill_task(current_task);
}
