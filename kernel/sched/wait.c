#include <types.h>
#include <error.h>

#include <kernel/mem.h>
#include <kernel/sched.h>

/* http://poincare.matf.bg.ac.rs/~ivana/courses/ps/sistemi_knjige/pomocno/apue/APUE/0201433079/ch08lev1sec6.html*/
/* If a child has already terminated and is a zombie,
 * wait returns immediately with that child's status.
 * Otherwise, it blocks the caller until a child terminates.
 * If the caller blocks and has multiple children,
 * wait returns when one terminates.
 * */
pid_t sys_wait(int *rstatus)
{
	/* LAB 5: your code here. */
	struct list *head;
	struct task *zombie;
	pid_t pid;

	if(rstatus == NULL) {
		return -ECHILD;
	}

	head = list_head(&cur_task->task_zombies);
	if (head) {
		zombie = container_of(head, struct task, task_node);
		pid = zombie->task_pid;

		// Found a zombie child, destroy it.
		cprintf("[PID %5u] Reaping task with PID %u\n", cur_task ? cur_task->task_pid : 0,
				zombie->task_pid);
		task_free(zombie);

		return pid;
	}

	cur_task->task_status = TASK_NOT_RUNNABLE;
	cur_task = NULL;
	sched_yield();

	return -ECHILD;
}

pid_t sys_waitpid(pid_t pid, int *rstatus, int opts)
{
	/* LAB 5: your code here. */
	struct task *task;

	task = pid2task(pid, 1);
	if(!task || task == cur_task) {
		return -ECHILD;
	}

	if(task->task_status == TASK_DYING) {
		// The process is a zombie, destroy it.
		cprintf("[PID %5u] Reaping task with PID %u\n", cur_task ? cur_task->task_pid : 0,
				task->task_pid);
		task_free(task);
		return pid;
	}

	cur_task->task_wait = task;
	cur_task->task_status = TASK_NOT_RUNNABLE;
	cur_task = NULL;
	sched_yield();

	return -ECHILD;
}

