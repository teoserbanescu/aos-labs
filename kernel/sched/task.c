#include <error.h>
#include <string.h>
#include <paging.h>
#include <task.h>
#include <cpu.h>

#include <kernel/monitor.h>
#include <kernel/mem.h>
#include <kernel/sched.h>
#include <kernel/vma.h>

#define RUN_TIME_BUDGET 1000000000

pid_t pid_max = 1 << 16;
struct task **tasks = (struct task **)PIDMAP_BASE;
size_t nuser_tasks = 0;

extern struct list runq;

int insert_task(struct task *task) {
	list_push(&runq, &task->task_node);
	return 0;
}

/* This function chooses the task that executed the least on the CPU
 * A red black tree or a priority queue would be a better approach,
 * but we choose to iterate through the list for now.
 * */
struct task* get_task() {
	struct task *task, *task_min;
	struct list *node;
	uint64_t min_time = -1;

	task_min = NULL;
	list_foreach(&runq, node) {
		task = container_of(node, struct task, task_node);
		if (task->task_runtime < min_time) {
			min_time = task->task_runtime;
			task_min = task;
		}
	}

	if (task_min) {
		list_remove(&task_min->task_node);
	}
	return task_min;
}

/* Looks up the respective task for a given PID.
 * If check_perm is non-zero, this function checks if the PID maps to the
 * current task or if the current task is the parent of the task that the PID
 * maps to.
 */
struct task *pid2task(pid_t pid, int check_perm)
{
	struct task *task;

	/* PID 0 is the current task. */
	if (pid == 0) {
		return cur_task;
	}

	/* Limit the PID. */
	if (pid < 0 || pid >= pid_max) {
		return NULL;
	}

	/* Look up the task in the PID map. */
	task = tasks[pid];

	/* No such mapping found. */
	if (!task) {
		return NULL;
	}

	/* If we don't have to do a permission check, we can simply return the
	 * task.
	 */
	if (!check_perm) {
		return task;
	}

	/* Check if the task is the current task or if the current task is the
	 * parent. If not, then the current task has insufficient permissions.
	 */
	if (task != cur_task && task->task_ppid != cur_task->task_pid) {
		return NULL;
	}

	return task;
}

void task_init(void)
{
	/* Allocate an array of pointers at PIDMAP_BASE to be able to map PIDs
	 * to tasks.
	 */
	/* LAB 3: your code here. */
	populate_region(kernel_pml4, (void *)PIDMAP_BASE,  sizeof(uintptr_t) * pid_max, PAGE_PRESENT | PAGE_WRITE | PAGE_NO_EXEC);
}

/* Sets up the virtual address space for the task. */
static int task_setup_vas(struct task *task)
{
	struct page_info *page;

	/* Allocate a page for the page table. */
	page = page_alloc(ALLOC_ZERO);

	if (!page) {
		return -ENOMEM;
	}

	++page->pp_ref;

	/* Now set task->task_pml4 and initialize the page table.
	 * Can you use kernel_pml4 as a template?
	 */

	/* LAB 3: your code here. */
    task->task_pml4 = page2kva(page);
/* https://canvas.vu.nl/courses/49330/discussion_topics/288564
 * just copying the pml4 entries (pointing to the same page tables as the kernel_pml4) should be enough.
 */
	for (int i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		task->task_pml4->entries[i] = kernel_pml4->entries[i];
	}

	return 0;
}
/* Find a free PID for the task in the PID mapping and associate the
 * task with that PID.
 */
int task_setup_pid(struct task *task) {
	pid_t pid;

	for (pid = 1; pid < pid_max; ++pid) {
		if (!tasks[pid]) {
			tasks[pid] = task;
			task->task_pid = pid;
			break;
		}
	}

	/* We are out of PIDs. */
	if (pid == pid_max) {
		kfree(task);
		return -1;
	}

	cprintf("[PID %5u] New task with PID %u\n",
			cur_task ? cur_task->task_pid : 0, task->task_pid);

	return 0;
}

void task_init_frame(struct task *task) {
	memset(&task->task_frame, 0, sizeof task->task_frame);

	task->task_frame.ds = GDT_UDATA | 3;
	task->task_frame.ss = GDT_UDATA | 3;
	task->task_frame.rsp = USTACK_TOP;
	task->task_frame.cs = GDT_UCODE | 3;
	task->task_frame.rflags = FLAGS_IF;
}

/* Allocates and initializes a new task.
 * On success, the new task is returned.
 */
struct task *task_alloc(pid_t ppid)
{
	struct task *task;

	/* Allocate a new task struct. */
	task = kmalloc(sizeof *task);

	if (!task) {
		return NULL;
	}

	/* Set up the virtual address space for the task. */
	if (task_setup_vas(task) < 0) {
		kfree(task);
		return NULL;
	}

	if (task_setup_pid(task) < 0) {
		kfree(task);
		return NULL;
	}

	/* Set up the task. */
	task->task_ppid = ppid;
	task->task_type = TASK_TYPE_USER;
	task->task_status = TASK_RUNNABLE;
	task->task_runs = 0;

	task_init_frame(task);

	rb_init(&task->task_rb);
	list_init(&task->task_mmap);
	list_init(&task->task_node);
	list_init(&task->task_children);
	list_init(&task->task_child);
	list_init(&task->task_zombies);

	/* You will set task->task_frame.rip later. */

	return task;
}

/* Sets up the initial program binary, stack and processor flags for a user
 * process.
 * This function is ONLY called during kernel initialization, before running
 * the first user-mode environment.
 *
 * This function loads all loadable segments from the ELF binary image into the
 * task's user memory, starting at the appropriate virtual addresses indicated
 * in the ELF program header.
 * At the same time it clears to zero any portions of these segments that are
 * marked in the program header as being mapped but not actually present in the
 * ELF file, i.e., the program's .bss section.
 *
 * All this is very similar to what our boot loader does, except the boot
 * loader also needs to read the code from disk. Take a look at boot/main.c to
 * get some ideas.
 *
 * Finally, this function maps one page for the program's initial stack.
 */
void task_load_elf(struct task *task, uint8_t *binary)
{
	/* Hints:
	 * - Load each program segment into virtual memory at the address
	 *   specified in the ELF section header.
	 * - You should only load segments with type ELF_PROG_LOAD.
	 * - Each segment's virtual address can be found in p_va and its
	 *   size in memory can be found in p_memsz.
	 * - The p_filesz bytes from the ELF binary, starting at binary +
	 *   p_offset, should be copied to virtual address p_va.
	 * - Any remaining memory bytes should be zero.
	 * - Use populate_region() and protect_region().
	 * - Check for malicious input.
	 *
	 * Loading the segments is much simpler if you can move data directly
	 * into the virtual addresses stored in the ELF binary.
	 * So in which address space should we be operating during this
	 * function?
	 *
	 * You must also do something with the entry point of the program, to
	 * make sure that the task starts executing there.
	 */

	/* LAB 3: your code here. */
	struct elf *elf_hdr;
	struct elf_proghdr *ph;
	size_t i;
	uint64_t flags;

	elf_hdr = (struct elf *)binary;
	ph = (struct elf_proghdr *)((uint8_t *)elf_hdr + elf_hdr->e_phoff);
	load_pml4((void *)PADDR(task->task_pml4)); // has to be here to reload address space


	for (i = 0; i < elf_hdr->e_phnum; ++i, ++ph) {
		if (ph->p_type != ELF_PROG_LOAD || ph->p_va == 0 || ph->p_memsz == 0) {
			continue;
		}

		flags = VM_READ;
		if (ph->p_flags & ELF_PROG_FLAG_WRITE) {
			flags |= VM_WRITE;
			add_anonymous_vma(task, ".data", (void *)ph->p_va, ph->p_memsz, flags);
		}
		else if (ph->p_flags & ELF_PROG_FLAG_EXEC) {
			flags |= VM_EXEC;
			add_executable_vma(task, ".text", (void *)ph->p_va, ph->p_memsz, flags,
					  (void *)binary + ph->p_offset, ph->p_filesz);
		}
		else {
			add_executable_vma(task, ".rodata", (void *)ph->p_va, ph->p_memsz, flags,
							   (void *)binary + ph->p_offset, ph->p_filesz);
		}
	}

	/* Now map one page for the program's initial stack at virtual address
	 * USTACK_TOP - PAGE_SIZE.
	 */

	/* LAB 3: your code here. */
    flags = VM_READ | VM_WRITE;
    add_anonymous_vma(task, "stack", (void *) USTACK_TOP - PAGE_SIZE, PAGE_SIZE, flags);

	task->task_frame.rsp = USTACK_TOP;
	task->task_frame.rip = elf_hdr->e_entry;
//	dump_page_tables(task->task_pml4, PAGE_PRESENT);
}

/* Allocates a new task with task_alloc(), loads the named ELF binary using
 * task_load_elf() and sets its task type.
 * If the task is a user task, increment the number of user tasks.
 * This function is ONLY called during kernel initialization, before running
 * the first user-mode task.
 * The new task's parent PID is set to 0.
 */
void task_create(uint8_t *binary, enum task_type type)
{
	/* LAB 3: your code here. */
	struct task *task;

	task = task_alloc(0);
	task_load_elf(task, binary);
	if (task->task_type == TASK_TYPE_USER) {
		nuser_tasks++;
	}
	/* LAB 5: your code here. */
//	list_push(&runq, &task->task_node);
	insert_task(task);
}

static void task_make_orphans(struct task *task) {
	struct list *node, *next;
	struct task *zombie;
	struct task *child;

	// Reap all zombies.
	if (!list_is_empty(&task->task_zombies)) {
		list_foreach_safe(&task->task_zombies, node, next) {
			zombie = container_of(node, struct task, task_node);

			// Found a zombie child, destroy it.
			cprintf("[PID %5u] Reaping task with PID %u\n", cur_task ? cur_task->task_pid : 0,
					zombie->task_pid);
			task_free(zombie);
		}
	}

	// Detach all children.
	if (!list_is_empty(&task->task_children)) {
		list_foreach_safe(&task->task_children, node, next) {
			child = container_of(node, struct task, task_child);
			list_remove(&child->task_child);
			child->task_ppid = 0;
		}
	}
}

/* Free the task and all of the memory that is used by it.
 */
void task_free(struct task *task)
{
	struct task *waiting;

	/* LAB 5: your code here. */
	task_make_orphans(task);

	/* If we are freeing the current task, switch to the kernel_pml4
	 * before freeing the page tables, just in case the page gets re-used.
	 */
	if (task == cur_task) {
		load_pml4((struct page_table *)PADDR(kernel_pml4));
	}

	/* Unmap the task from the PID map. */
	tasks[task->task_pid] = NULL;

	list_remove(&task->task_child);
	list_remove(&task->task_node);

	/* Unmap the user pages. */
	free_vmas(task);
	unmap_user_pages(task->task_pml4);

	/* Note the task's demise. */
	cprintf("[PID %5u] Freed task with PID %u\n", cur_task ? cur_task->task_pid : 0,
	    task->task_pid);

	nuser_tasks--;

	/* Free the task. */
	kfree(task);
}

static void task_zombie(struct task *task) {
	struct task *parent;

	parent = pid2task(task->task_ppid, 0);

	if (!parent || task->task_ppid == 0) {
		task_free(task);
		return;
	}

//	parent is waiting for us
	if(parent->task_status == TASK_NOT_RUNNABLE &&
			parent->task_wait == task) {
		parent->task_frame.rax = task->task_pid;
		parent->task_status = TASK_RUNNABLE;
//		list_push(&runq, &parent->task_node);
		insert_task(parent);
		task_free(task);
	}
	else {
		task->task_status = TASK_DYING;
		// remove from the run queue
		list_remove(&task->task_node);
		list_push(&parent->task_zombies, &task->task_node);
	}
}

/* Frees the task. If the task is the currently running task, then this
 * function runs a new task (and does not return to the caller).
 */
void task_destroy(struct task *task)
{
	/* LAB 5: your code here. */
	int curr;

	curr = task == cur_task;
	task_zombie(task);

	if (curr) {
		cur_task = NULL;
		sched_yield();
	}
}

/*
 * Restores the register values in the trap frame with the iretq or sysretq
 * instruction. This exits the kernel and starts executing the code of some
 * task.
 *
 * This function does not return.
 */
void task_pop_frame(struct int_frame *frame)
{
#ifdef BONUS_LAB3
	MDS_buff_overwrite();	// flush CPU buffers
#endif

	switch (frame->int_no) {
#ifdef LAB3_SYSCALL
	case 0x80: sysret64(frame); break;
#endif
	default: iret64(frame); break;
	}

	panic("We should have gone back to userspace!");
}

/* Context switch from the current task to the provided task.
 * Note: if this is the first call to task_run(), cur_task is NULL.
 *
 * This function does not return.
 */
void task_run(struct task *task)
{
	/*
	 * Step 1: If this is a context switch (a new task is running):
	 *     1. Set the current task (if any) back to
	 *        TASK_RUNNABLE if it is TASK_RUNNING (think about
	 *        what other states it can be in),
	 *     2. Set 'cur_task' to the new task,
	 *     3. Set its status to TASK_RUNNING,
	 *     4. Update its 'task_runs' counter,
	 *     5. Use load_pml4() to switch to its address space.
	 * Step 2: Use task_pop_frame() to restore the task's
	 *     registers and drop into user mode in the
	 *     task.
	 *
	 * Hint: This function loads the new task's state from
	 *  e->task_frame.  Go back through the code you wrote above
	 *  and make sure you have set the relevant parts of
	 *  e->task_frame to sensible values.
	 */

	/* LAB 3: Your code here. */
//	panic("task_run() not yet implemented");

//	cprintf("Running task with PID %u\n", cur_task ? cur_task->task_pid : 0);

	if (!cur_task) {
		cur_task = task;
	}

	if (cur_task->task_status == TASK_RUNNING) {
		cur_task->task_status = TASK_RUNNABLE;
	}

	cur_task = task;
	task->task_status = TASK_RUNNING;
	task->task_runs++;
	load_pml4((void *)PADDR(task->task_pml4));

	task_pop_frame(&task->task_frame);
}

