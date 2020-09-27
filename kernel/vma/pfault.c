#include <types.h>

#include <kernel/mem.h>
#include <kernel/vma.h>

/* Handles the page fault for a given task. */
int task_page_fault_handler(struct task *task, void *va, int flags)
{
	/* LAB 4: your code here. */
	struct vma *vma;

	vma = find_vma(NULL, NULL, &task->task_rb, va);
	if (va < vma->vm_base || va > vma->vm_end || vma->vm_flags == 0) {
	    return -1;
	}

	return populate_vma_range(task, ROUNDDOWN(va, PAGE_SIZE), PAGE_SIZE, flags);
}

