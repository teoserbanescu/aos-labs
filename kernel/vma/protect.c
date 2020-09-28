#include <types.h>

#include <kernel/mem.h>
#include <kernel/vma.h>

#include <kernel/mem/protect.h>

#include <lib.h>

/* Changes the protection flags of the given VMA. Does nothing if the flags
 * would remain the same. Splits up the VMA into the address range
 * [base, base + size) and changes the protection of the physical pages backing
 * the VMA. Then attempts to merge the VMAs in case the protection became the
 * same as that of any of the adjacent VMAs.
 */
int do_protect_vma(struct task *task, void *base, size_t size, struct vma *vma,
	void *udata)
{
	/* LAB 4 (bonus): your code here. */
	struct vma *vma_to_protect;
    uint64_t vm_flags, page_flags;
    int prot_flags;

    prot_flags = *((uintptr_t *)udata);

    // Some configurations of the protection flags are not supported on x86-64.
    if ((prot_flags == PROT_WRITE) ||
        (prot_flags == PROT_EXEC) ||
        ((prot_flags & (PROT_WRITE | PROT_EXEC)) == (PROT_WRITE | PROT_EXEC))) {
        return -1;
    }


    vma_to_protect = split_vmas(task, vma, base, size);

    vm_flags = 0;
    vm_flags |= (prot_flags & PROT_READ) ? VM_READ : 0;
    vm_flags |= (prot_flags & PROT_WRITE) ? VM_WRITE : 0;
    vm_flags |= (prot_flags & PROT_EXEC) ? VM_EXEC : 0;
    vma_to_protect -> vm_flags = vm_flags;

    page_flags = 0;
    page_flags |= !(prot_flags == PROT_NONE) ? (PAGE_USER | PAGE_PRESENT) : 0;
    page_flags |= (vma->vm_flags & VM_WRITE) ? PAGE_WRITE : 0;
    page_flags |= !(vma->vm_flags & VM_EXEC) ? PAGE_NO_EXEC : 0;

    protect_region(task->task_pml4, base, size, page_flags);

	return 0;
}

/* Changes the protection flags of the VMAs for the given address range
 * [base, base + size).
 */
int protect_vma_range(struct task *task, void *base, size_t size, int flags)
{
	return walk_vma_range(task, base, size, do_protect_vma, &flags);
}

