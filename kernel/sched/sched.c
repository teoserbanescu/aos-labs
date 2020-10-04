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

	if (cur_task) {
		list_push(&runq, &cur_task->task_node);
	}

	node = list_pop(&runq);
	if (node != NULL) {
		task = container_of(node, struct task, task_node);
		task_run(task);
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

