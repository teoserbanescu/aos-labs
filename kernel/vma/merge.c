#include <task.h>
#include <vma.h>

#include <kernel/vma.h>
#include <include/kernel/mem.h>

/* Given a task and two VMAs, checks if the VMAs are adjacent and compatible
 * for merging. If they are, then the VMAs are merged by removing the
 * right-hand side and extending the left-hand side by setting the end address
 * of the left-hand side to the end address of the right-hand side.
 */
struct vma *merge_vma(struct task *task, struct vma *lhs, struct vma *rhs)
{
	/* LAB 4: your code here. */
	if(lhs->vm_end == rhs->vm_base && lhs->vm_flags == rhs->vm_flags) {
		lhs->vm_end = rhs->vm_end;
		remove_vma(task, rhs);
		kfree(rhs);
		return lhs;
	}

	return NULL;
}

/* Given a task and a VMA, this function attempts to merge the given VMA with
 * the previous and the next VMA. Returns the merged VMA or the original VMA if
 * the VMAs could not be merged.
 */
struct vma *merge_vmas(struct task *task, struct vma *vma)
{
	/* LAB 4: your code here. */
	struct vma *prev, *next, *merged;

	prev = container_of(vma->vm_mmap.prev, struct vma, vm_mmap);
	next = container_of(vma->vm_mmap.next, struct vma, vm_mmap);

	merged = merge_vma(task, prev, vma);
	if(merged)
		vma = merged;
	merged = merge_vma(task, vma, next);
	if(merged)
		vma = merged;

	return vma;
}

