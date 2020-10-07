#include <types.h>
#include <cpu.h>
#include <list.h>
#include <stdio.h>

#include <x86-64/asm.h>
#include <x86-64/paging.h>

#include <kernel/mem.h>
#include <kernel/monitor.h>
#include <kernel/sched.h>

struct list runq;

#ifndef USE_BIG_KERNEL_LOCK
struct spinlock runq_lock = {
#ifdef DBEUG_SPINLOCK
	.name = "runq_lock",
#endif
};
#endif

extern size_t nuser_tasks;

void sched_init(void)
{
	list_init(&runq);
}

void sched_init_mp(void)
{
	/* LAB 6: your code here. */
}

/* Runs the next runnable task. */
void sched_yield(void)
{
	/* LAB 5: your code here. */
	struct list *node;
	struct task *task;

	if (cur_task) {
		cur_task->task_runtime = read_tsc() - cur_task->task_start_time;
		insert_task(cur_task);
	}

	if (!list_is_empty(&runq)) {
		task = get_task();
		task->task_start_time = read_tsc();
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

