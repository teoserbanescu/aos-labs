#include <types.h>
#include <cpu.h>
#include <list.h>
#include <stdio.h>

#include <x86-64/asm.h>
#include <x86-64/paging.h>

#include <kernel/mem.h>
#include <kernel/monitor.h>
#include <kernel/sched.h>

#include <spinlock.h>

// global run list
struct list runq;
extern size_t nuser_tasks;

#ifndef USE_BIG_KERNEL_LOCK
struct spinlock runq_lock = {
#ifdef DBEUG_SPINLOCK
	.name = "runq_lock",
#endif
};
#endif


void sched_init(void)
{
	list_init(&runq);
	sched_init_mp();
}

void sched_init_mp(void)
{
	/* LAB 6: your code here. */
	list_init(&this_cpu->runq);
	list_init(&this_cpu->nextq);
	this_cpu->runq_len = 0;
}

/* Runs the next runnable task. */
void sched_yield(void)
{
	/* LAB 5: your code here. */
	struct list *node;
	struct task *task;

#ifdef USE_BIG_KERNEL_LOCK
	spin_lock(&kernel_lock);
#endif
	if (cur_task) {
		cur_task->task_runtime = read_tsc() - cur_task->task_start_time;
		insert_task(cur_task);
	}

	task = get_task();
#ifdef USE_BIG_KERNEL_LOCK
	spin_unlock(&kernel_lock);
#endif

	if (task) {
		task->task_start_time = read_tsc();
		task_run(task);
	}
	else {
		sched_halt();
	}

	panic("yield returned");
}

/* For now jump into the kernel monitor. */
void sched_halt()
{
	this_cpu->cpu_status = CPU_HALTED;

    cprintf("Destroyed the only task - nothing more to do!\n");

	asm volatile(
	"cli\n"
	"hlt\n");

    while (1) {
		monitor(NULL);
	}
}

