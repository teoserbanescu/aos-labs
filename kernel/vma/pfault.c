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

		*entry |= PAGE_WRITE;
		page_copy = page_alloc(BUDDY_4K_PAGE);
		page_copy->pp_ref++;
		memcpy(page2kva(page_copy), page2kva(page), PAGE_SIZE);

		*entry = PAGE_ADDR(page2pa(page_copy)) | (*entry & PAGE_MASK);
		*entry |= PAGE_WRITE;
		tlb_invalidate(task->task_pml4, va);
		return 0;
	}
	/* End COW */

	return populate_vma_range(task, ROUNDDOWN(va, PAGE_SIZE), PAGE_SIZE, flags);
	/* LAB 5: your code here. */
}

