#include <types.h>
#include <list.h>
#include <stdio.h>

#include <x86-64/asm.h>
#include <x86-64/paging.h>

#include <kernel/mem.h>
#include <kernel/monitor.h>
#include <kernel/sched.h>

struct list runq;

extern size_t nuser_tasks;

void sched_init(void)
{
	list_init(&runq);
}

/* Runs the next runnable task. */
void sched_yield(void)
{
	/* LAB 5: your code here. */
	struct list *node;
	struct task *task;

	// FIXME loop through all tasks and find the one with the biggest priority
	// based on task_time_left.
	if (cur_task && cur_task->task_status == TASK_RUNNING) {
		cur_task->task_time_left = cur_task->task_time_budget -
				(read_tsc() - cur_task->task_start_time);
		if ((cur_task->task_time_left > 0) && (!cur_task->task_preempted)) {
			list_push_left(&runq, &cur_task->task_node);
		} else {
			list_push(&runq, &cur_task->task_node);
		}
	}

	if (!list_is_empty(&runq)) {
		node = list_pop(&runq);
		if (node != NULL) {
			task = container_of(node, struct task, task_node);
			task->task_start_time = read_tsc();
			task->task_time_left = task->task_time_budget;
			task->task_preempted = 0;
			task_run(task);
		}
	}

	if (list_is_empty(&runq)) {
		sched_halt();
	}

	panic("yield returned");
}

/* For now jump into the kernel monitor. */
void sched_halt()
{
    cprintf("Destroyed the only task - nothing more to do!\n");

    while (1) {
		monitor(NULL);
	}
}

