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
	struct task *next_task;

	node = list_pop_left(&runq);
	if (node != NULL) {
        next_task = container_of(node, struct task, task_node);
        task_run(next_task);
        // FIXME is this condition right?
    } else if (nuser_tasks == 0) {
	    sched_halt();
	}
}

/* For now jump into the kernel monitor. */
void sched_halt()
{
    cprintf("Destroyed the only task - nothing more to do!\n");

    while (1) {
		monitor(NULL);
	}
}

