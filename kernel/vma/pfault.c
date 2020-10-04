#include <types.h>

#include <kernel/mem.h>
#include <kernel/vma.h>

/* Handles the page fault for a given task. */
int task_page_fault_handler(struct task *task, void *va, int flags)
{
	/* LAB 4: your code here. */
	struct vma *vma;
	physaddr_t *entry;
	struct page_info *page;
	struct page_info *page_copy;

	vma = find_vma(NULL, NULL, &task->task_rb, va);
	if (va < vma->vm_base || va > vma->vm_end || vma->vm_flags == 0) {
	    return -1;
	}

	/* COW */
	page = page_lookup(task->task_pml4, va, &entry);
	if(entry && page &&
		!(*entry & PAGE_WRITE) &&
		(vma->vm_flags & VM_WRITE)) {
		/* FIXME do cow */
	}
	/* End COW */

	return populate_vma_range(task, ROUNDDOWN(va, PAGE_SIZE), PAGE_SIZE, flags);
	/* LAB 5: your code here. */
}

