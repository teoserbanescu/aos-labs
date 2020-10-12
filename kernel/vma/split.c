#include <task.h>
#include <vma.h>

#include <kernel/vma.h>

/* Given a task and a VMA, this function splits the VMA at the given address
 * by setting the end address of original VMA to the given address and by
 * adding a new VMA with the given address as base.
 */
struct vma *split_vma(struct task *task, struct vma *lhs, void *addr)
{
	/* LAB 4: your code here. */
	struct vma *rhs;

    if (lhs->vm_base == addr || lhs->vm_end == addr) {
        return lhs;
    }

    rhs = add_anonymous_vma(task, lhs->vm_name, addr,
                         lhs->vm_end - addr, lhs->vm_flags);

    lhs->vm_end = addr;

	return rhs;
}

/* Given a task and a VMA, this function first splits the VMA into a left-hand
 * and right-hand side at address base. Then this function splits the
 * right-hand side or the original VMA, if no split happened, into a left-hand
 * and a right-hand side. This function finally returns the right-hand side of
 * the first split or the original VMA.
 */
struct vma *split_vmas(struct task *task, struct vma *vma, void *base, size_t size)
{
	/* LAB 4: your code here. */
	struct vma *middle, *rhs;

	middle = split_vma(task, vma, base);
	rhs = split_vma(task, middle, base + size);

	return middle;
}

