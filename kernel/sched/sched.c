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
volatile size_t runq_len = 0;
extern volatile size_t nuser_tasks;

//#ifndef USE_BIG_KERNEL_LOCK
struct spinlock runq_lock = {
#ifdef DEBUG_SPINLOCK
	.name = "runq_lock",
#endif
};
//#endif

/* Calculate an average of how many tasks a cpu should have
 * If current cpu has more than average then try to push the new task in the global queue
 * */
int insert_task(struct task *task) {
	size_t n;

	n = ROUNDUP(runq_len, ncpus) / ncpus;

	int x = this_cpu->runq_len;
	int y = this_cpu->nextq_len;

	if (this_cpu->runq_len + this_cpu->nextq_len > n) {
		if (spin_trylock(&runq_lock)) {
			list_push_left(&runq, &task->task_node);
			runq_len++;
			spin_unlock(&runq_lock);
			return 0;
		}
	}

	list_push_left(&this_cpu->nextq, &task->task_node);
	this_cpu->nextq_len++;
	return 0;
}

/* This function chooses the task that executed the least on the CPU
 * TODO replace runqueues lists with red black tree
 * */
struct task* get_min_task() {
	struct task *task, *task_min;
	struct list *node;
	uint64_t min_time = -1;
	struct list *lrunq;

	lrunq = &this_cpu->runq;

	if (list_is_empty(lrunq)) {
		return NULL;
	}

	task_min = NULL;
	list_foreach(lrunq, node) {
		task = container_of(node, struct task, task_node);
		if (task->task_runtime < min_time) {
			min_time = task->task_runtime;
			task_min = task;
		}
	}

	if (task_min) {
		list_remove(&task_min->task_node);
		this_cpu->runq_len--;
	}
	return task_min;
}

static void migrate() {
	size_t n, i;
	struct task *task;

/*	busywait as long as we have tasks in the global queue
 *	or until all of tasks were run
 */
	while(nuser_tasks && !runq_len);

	if (!runq_len){
		return;
	}

//	TODO try with nuser_tasks or runq_len
	n = ROUNDUP(runq_len, ncpus) / ncpus;

	spin_lock(&runq_lock);

	runq_len -= n;

	for (i = 0; i < n; ++i) {
		task = container_of(list_pop(&runq), struct task, task_node);
		list_push_left(&this_cpu->runq, &task->task_node);
	}

	spin_unlock(&runq_lock);

	this_cpu->runq_len += n;
}

struct task* get_task() {
	struct task *task;
	struct list *node;

//	swap if necessary
	if(list_is_empty(&this_cpu->runq) && !list_is_empty(&this_cpu->nextq)) {
		node = list_head(&this_cpu->nextq);
		list_remove(&this_cpu->nextq);
		list_push_left(node, &this_cpu->runq);
		this_cpu->runq_len = this_cpu->nextq_len;
		this_cpu->nextq_len = 0;
	}

	if (this_cpu->runq_len == 0) {
		migrate();
	}

	task = get_min_task();
	return task;
}

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
	this_cpu->nextq_len = 0;
}

/* Runs the next runnable task. */
void sched_yield(void)
{
	/* LAB 5: your code here. */
	struct task *task = NULL;

	if (cur_task) {
		cur_task->task_runtime = read_tsc() - cur_task->task_start_time;
		insert_task(cur_task);
	}

	task = get_task();

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
	int i;

	this_cpu->cpu_status = CPU_HALTED;

	cprintf("CPU %u halted!\n", this_cpu->cpu_id);

/*	if there is one more cpu which is not halted yet, halt this one
 *  else monitor this one
 */
	for (i = 0; i < ncpus; ++i) {
		if (cpus[i].cpu_status != CPU_HALTED) {
#ifdef USE_BIG_KERNEL_LOCK
			spin_unlock(&kernel_lock);
#endif
			asm volatile(
			"cli\n"
			"hlt\n");
		}
	}

	cprintf("Destroyed the only task - nothing more to do!\n");

#ifdef USE_BIG_KERNEL_LOCK
	spin_unlock(&kernel_lock);
#endif
	while (1) {
		monitor(NULL);
	}
}

