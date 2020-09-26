#include <types.h>

#include <kernel/mem.h>
#include <kernel/vma.h>

/* Checks the flags in udata against the flags of the VMA to check appropriate
 * permissions. If the permissions are all right, this function populates the
 * address range [base, base + size) with physical pages. If the VMA is backed
 * by an executable, the data is copied over. Then the protection of the
 * physical pages is adjusted to match the permissions of the VMA.
 */
int do_populate_vma(struct task *task, void *base, size_t size,
	struct vma *vma, void *udata)
{
	/* LAB 4: your code here. */
    uint64_t flags;
//		flags = PAGE_USER | PAGE_PRESENT;
//		flags |= (ph->p_flags & ELF_PROG_FLAG_WRITE) ? PAGE_WRITE : 0;
//		flags |= !(ph->p_flags & ELF_PROG_FLAG_EXEC) ? PAGE_NO_EXEC : 0;
//		populate_region(task->task_pml4, (void *)ph->p_va, ph->p_memsz, flags);
//
//		memcpy((void *)ph->p_va, binary + ph->p_offset, ph->p_filesz);
//		protect_region(task->task_pml4, (void*)ph->p_va, ph->p_memsz, flags);
    flags = PAGE_USER | PAGE_PRESENT;
    flags |= (vma->vm_flags & VM_WRITE) ? PAGE_WRITE : 0;
    flags |= !(vma->vm_flags & VM_EXEC) ? PAGE_NO_EXEC : 0;

    populate_region(task->task_pml4, base, size, flags);
    // FIXME memcpy if vma->vm_src is present.
    protect_region(task->task_pml4, base, size, flags);
	return 0;
}

/* Populates the VMAs for the given address range [base, base + size) by
 * backing the VMAs with physical pages.
 */
int populate_vma_range(struct task *task, void *base, size_t size, int flags)
{
	return walk_vma_range(task, base, size, do_populate_vma, &flags);
}

