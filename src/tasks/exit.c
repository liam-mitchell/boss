#include "task.h"
#include "internal.h"

#include "printf.h"

#include "memory/kheap.h"
#include "fs/vfs.h"

static void kill_task(struct task *task);

static void kill_children_on_queue(struct task **queue, struct task *task)
{
    if (*queue == NULL) {
	return;
    }
	
    for (struct task *curr = *queue; curr; curr = curr->next) {
	if (curr->parent == task) {
	    kill_task(curr);
	}
    }
}

static void kill_children(struct task *task)
{
    kill_children_on_queue(&running, task);
    kill_children_on_queue(&blocked, task);
    kill_children_on_queue(&zombies, task);
}

static void kill_task(struct task *task)
{
    kill_children(task);

    for (int i = 0; i < TASK_MAX_FILES; ++i) {
	if (task->files[i]) {
	    printf("closing process file %d\n", i);
	    vfs_close(task->files[i]);
	}
    }

    task_queue_remove_safe(&running, task);
    task_queue_remove_safe(&blocked, task);
    task_queue_remove_safe(&zombies, task);
}

void exit(int code)
{
    printf("exiting process %u with code %d\n", current_task->pid, code);

    current_task->exit_code = code;
    current_task->status = TASK_FINISHED;

    if (current_task->parent->status == TASK_BLOCKED) {
	wake(current_task->parent->pid);
    }

    kill_task(current_task);
}
