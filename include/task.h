#pragma once

#include <types.h>
#include <list.h>
#include <rbtree.h>
#include <spinlock.h>

#include <x86-64/idt.h>
#include <x86-64/memory.h>

typedef int32_t pid_t;

int task_setup_pid();

/* Special task types. */
enum task_type {
	TASK_TYPE_USER = 0,
	TASK_TYPE_KERNEL,
};

struct task *task_alloc(pid_t ppid, enum task_type type);

/* Values of task_status in struct task. */
enum {
	TASK_DYING = 0,
	TASK_RUNNABLE,
	TASK_RUNNING,
	TASK_NOT_RUNNABLE,
};

/* The method of interrupt used to switch to the kernel. */
enum {
	TASK_INT = 0,
	TASK_SYSCALL,
};

struct task {
	/* The saved registers. */
	struct int_frame task_frame;

	/* The saved registers for when we yield from kernel */
	struct int_frame ktask_frame;

	/* The task this task is waiting on. */
	struct task *task_wait;

	/* The process ID of this task and its parent. */
	pid_t task_pid;
	pid_t task_ppid;

	/* The task type. */
	enum task_type task_type;

	/* The task status. */
	volatile unsigned task_status;

	/* The number of times the task has been run. */
	unsigned task_runs;

	/* The CPU that the task is running on. */
	int task_cpunum;

	/* The virtual address space. */
	struct page_table *task_pml4;

	/* The VMAs */
	struct rb_tree task_rb;
	struct list task_mmap;

	/* The children */
	struct list task_children;
	struct list task_child;

	/* The zombies */
	struct list task_zombies;

	/* The anchor node (for zombies or the run queue) */
	struct list task_node;

	uint64_t task_runtime;
	uint64_t task_start_time;

	/* Per-task lock */
	struct spinlock task_lock;

	bool kyield;
	uint32_t nactive_pages;
	uint32_t nswapped_pages;
};

void ksched_yield();
void asm_ksched_yield(struct int_frame *frame);
void task_init_frame(struct task *task, enum task_type type);

int insert_task(struct task *task);
struct task* get_task();


